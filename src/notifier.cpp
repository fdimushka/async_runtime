#include "ar/notifier.hpp"
#include <cassert>

using namespace AsyncRuntime;


Notifier::Notifier(size_t N) : _waiters{N}
{
    assert(_waiters.size() < (1 << kWaiterBits) - 1);
    _state = kStackMask | (kEpochMask - kEpochInc * _waiters.size() * 2);
}


Notifier::~Notifier()
{
    assert((_state.load() & (kStackMask | kWaiterMask)) == kStackMask);
}


void Notifier::PrepareWait(Waiter* w) {
    w->epoch = _state.fetch_add(kWaiterInc, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
}


void Notifier::Wait(Waiter* w) {
    w->state = WaiterState::kNotSignaled;
    // Modification epoch of this waiter.
    uint64_t epoch =
            (w->epoch & kEpochMask) +
            (((w->epoch & kWaiterMask) >> kWaiterShift) << kEpochShift);
    uint64_t state = _state.load(std::memory_order_seq_cst);
    for (;;) {
        if (int64_t((state & kEpochMask) - epoch) < 0) {
            // The preceding waiter has not decided on its fate. Wait until it
            // calls either CancelWait or Wait, or is notified.
            std::this_thread::yield();
            state = _state.load(std::memory_order_seq_cst);
            continue;
        }
        // We've already been notified.
        if (int64_t((state & kEpochMask) - epoch) > 0) return;
        // Remove this thread from prewait counter and add it to the waiter list.
        assert((state & kWaiterMask) != 0);
        uint64_t newstate = state - kWaiterInc + kEpochInc;
        //newstate = (newstate & ~kStackMask) | (w - &_waiters[0]);
        newstate = static_cast<uint64_t>((newstate & ~kStackMask) | static_cast<uint64_t>(w - &_waiters[0]));
        if ((state & kStackMask) == kStackMask)
            w->next.store(nullptr, std::memory_order_relaxed);
        else
            w->next.store(&_waiters[state & kStackMask], std::memory_order_relaxed);
        if (_state.compare_exchange_weak(state, newstate,
                                         std::memory_order_release))
            break;
    }
    Park(w);
}


void Notifier::CancelWait(Waiter* w) {
    uint64_t epoch =
            (w->epoch & kEpochMask) +
            (((w->epoch & kWaiterMask) >> kWaiterShift) << kEpochShift);
    uint64_t state = _state.load(std::memory_order_relaxed);
    for (;;) {
        if (int64_t((state & kEpochMask) - epoch) < 0) {
            // The preceeding waiter has not decided on its fate. Wait until it
            // calls either cancel_wait or commit_wait, or is notified.
            std::this_thread::yield();
            state = _state.load(std::memory_order_relaxed);
            continue;
        }
        // We've already been notified.
        if (int64_t((state & kEpochMask) - epoch) > 0) return;
        // Remove this thread from prewait counter.
        assert((state & kWaiterMask) != 0);
        if (_state.compare_exchange_weak(state, state - kWaiterInc + kEpochInc,
                                         std::memory_order_relaxed))
            return;
    }
}


void Notifier::Notify(bool all) {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    uint64_t state = _state.load(std::memory_order_acquire);
    for (;;) {
        // Easy case: no waiters.
        if ((state & kStackMask) == kStackMask && (state & kWaiterMask) == 0)
            return;
        uint64_t waiters = (state & kWaiterMask) >> kWaiterShift;

        uint64_t newstate;
        if (all) {
            // Reset prewait counter and empty wait list.
            newstate = (state & kEpochMask) + (kEpochInc * waiters) + kStackMask;
        } else if (waiters) {
            // There is a thread in pre-wait state, unblock it.
            newstate = state + kEpochInc - kWaiterInc;
        } else {
            // Pop a waiter from list and unpark it.
            Waiter* w = &_waiters[state & kStackMask];
            Waiter* wnext = w->next.load(std::memory_order_relaxed);
            uint64_t next = kStackMask;
            //if (wnext != nullptr) next = wnext - &_waiters[0];
            if (wnext != nullptr) next = static_cast<uint64_t>(wnext - &_waiters[0]);
            // Note: we don't add kEpochInc here. ABA problem on the lock-free stack
            // can't happen because a waiter is re-pushed onto the stack only after
            // it was in the pre-wait state which inevitably leads to epoch
            // increment.
            newstate = (state & kEpochMask) + next;
        }
        if (_state.compare_exchange_weak(state, newstate,
                                         std::memory_order_acquire)) {
            if (!all && waiters) return;  // unblocked pre-wait thread
            if ((state & kStackMask) == kStackMask) return;

            Waiter* w = &_waiters[state & kStackMask];
            if (!all) w->next.store(nullptr, std::memory_order_relaxed);
            UnPark(w);
            return;
        }
    }
}


void Notifier::NotifyOne(Waiter* w) {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    w->next.store(nullptr, std::memory_order_relaxed);
    UnPark(w);
}


void Notifier::NotifyMany(size_t n) {
    if(n >= _waiters.size()) {
        Notify(true);
    }
    else {
        for(size_t k=0; k<n; ++k) {
            Notify(false);
        }
    }
}


void Notifier::Park(Waiter* w) {
    std::unique_lock<std::mutex> lock(w->mu);
    while (w->state != WaiterState::kSignaled) {
        w->state = WaiterState::kWaiting;

        w->cv.wait(lock);
//        if(w->cv.wait_for(lock,std::chrono::milliseconds (1000)) == std::cv_status::timeout){
//            break;
//        }
    }
}


void Notifier::UnPark(Waiter* waiters) {
    Waiter* next = nullptr;
    for (Waiter* w = waiters; w; w = next) {
        next = w->next.load(std::memory_order_relaxed);
        unsigned state;
        {
            std::unique_lock<std::mutex> lock(w->mu);
            state = w->state;
            w->state = WaiterState::kSignaled;
        }
        // Avoid notifying if it wasn't waiting.
        if (state == WaiterState::kWaiting) {
            w->cv.notify_one();
        }
    }
}