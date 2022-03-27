#ifndef AR_CHANNEL_H
#define AR_CHANNEL_H

#include "object.hpp"
#include "work_steal_queue.h"


namespace AsyncRuntime {


    /**
     * @class Channel
     * @brief Lock-free unbounded single-producer single-consumer queue.
     * @tparam T
     */
    template<typename T>
    class Channel {
    public:


        /**
        @brief constructs the queue with a given capacity
        @param capacity the capacity of the queue (must be power of 2)
        */
        explicit Channel(int64_t capacity = 1024);


        /**
        @brief destructs the queue
        */
        ~Channel();


        /**
        @brief queries if the queue is empty at the time of this call
        */
        bool empty() const noexcept;


        /**
        @brief queries if the queue is full at the time of this call
        */
        bool full() const noexcept;


        /**
        @brief queries the number of items at the time of this call
        */
        size_t size() const noexcept;


        /**
        @brief queries the capacity of the queue
        */
        int64_t capacity() const noexcept;


        /**
        @brief inserts an item to the queue
        Only the owner thread can insert an item to the queue.
        The operation can trigger the queue to resize its capacity
        if more space is required.
        @tparam O data type
        @param item the item to perfect-forward to the queue
        */
        template<typename O>
        bool push(O &&item);


        /**
        @brief pops out an item from the queue
        Only the owner thread can pop out an item from the queue.
        The return can be a @std_nullopt if this operation failed (empty queue).
        */
        std::optional<T> pop();


    private:
        std::atomic<int64_t> _top;
        std::atomic<int64_t> _bottom;
        std::atomic<Array<T> *> _array;
        std::vector<Array<T> *> _garbage;
    };


    // Constructor
    template<typename T>
    Channel<T>::Channel(int64_t c) {
        assert(c && (!(c & (c - 1))));
        _top.store(0, std::memory_order_relaxed);
        _bottom.store(0, std::memory_order_relaxed);
        _array.store(new Array<T>{c}, std::memory_order_relaxed);
        _garbage.reserve(32);
    }

// Destructor
    template<typename T>
    Channel<T>::~Channel() {
        for (auto a: _garbage) {
            delete a;
        }
        delete _array.load();
    }


    template<typename T>
    bool Channel<T>::empty() const noexcept {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_relaxed);
        return b <= t;
    }


    template<typename T>
    bool Channel<T>::full() const noexcept {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_acquire);
        Array<T> *a = _array.load(std::memory_order_relaxed);
        return a->capacity() - 1 < (b - t);
    }

// Function: size
    template<typename T>
    size_t Channel<T>::size() const noexcept {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_relaxed);
        return static_cast<size_t>(b >= t ? b - t : 0);
    }

// Function: push
    template<typename T>
    template<typename O>
    bool Channel<T>::push(O &&o) {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_acquire);
        Array<T> *a = _array.load(std::memory_order_relaxed);

        // queue is full
        if (a->capacity() - 1 < (b - t)) {
            return false;
        }

        a->push(b, std::forward<O>(o));
        std::atomic_thread_fence(std::memory_order_release);
        _bottom.store(b + 1, std::memory_order_relaxed);

        return true;
    }

// Function: pop
    template<typename T>
    std::optional<T> Channel<T>::pop() {
        int64_t b = _bottom.load(std::memory_order_relaxed) - 1;
        Array<T> *a = _array.load(std::memory_order_relaxed);
        _bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t t = _top.load(std::memory_order_relaxed);

        std::optional<T> item;

        if (t <= b) {
            item = a->pop(b);
            if (t == b) {
                // the last item just got stolen
                if (!_top.compare_exchange_strong(t, t + 1,
                                                  std::memory_order_seq_cst,
                                                  std::memory_order_relaxed)) {
                    item = std::nullopt;
                }
                _bottom.store(b + 1, std::memory_order_relaxed);
            }
        } else {
            _bottom.store(b + 1, std::memory_order_relaxed);
        }

        return item;
    }


    /**
     * @brief
     * @tparam T
     * @return
     */
    template <typename T>
    static std::shared_ptr<Channel<T>> MakeChannel(size_t cap = INT_MAX) {
        return std::make_shared<Channel<T>>(cap);
    }
}

#endif //AR_CHANNEL_H
