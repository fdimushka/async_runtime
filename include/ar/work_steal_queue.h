#ifndef AR_WORKSTEALQUEUE_H
#define AR_WORKSTEALQUEUE_H


#include "os.hpp"
#include "array.hpp"


namespace AsyncRuntime {


    /**
     * @class TaskQueue
     * @brief Lock-free unbounded single-producer multiple-consumer queue.
     * @tparam T data type
     */
    template<typename T>
    class WorkStealQueue {
    public:

        /**
        @brief constructs the queue with a given capacity
        @param capacity the capacity of the queue (must be power of 2)
        */
        explicit WorkStealQueue(int64_t capacity = 1024);

        /**
        @brief destructs the queue
        */
        ~WorkStealQueue();

        /**
        @brief queries if the queue is empty at the time of this call
        */
        bool empty() const noexcept;

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
        void push(O &&item);

        /**
        @brief pops out an item from the queue
        Only the owner thread can pop out an item from the queue.
        The return can be a @std_nullopt if this operation failed (empty queue).
        */
        std::optional<T> pop();

        /**
        @brief steals an item from the queue
        Any threads can try to steal an item from the queue.
        The return can be a @std_nullopt if this operation failed (not necessary empty).
        */
        std::optional<T> steal();


    private:
        std::atomic<int64_t> _top;
        std::atomic<int64_t> _bottom;
        std::atomic<AtomicArray<T> *> _array;
        std::vector<AtomicArray<T> *> _garbage;
    };


    template<typename T>
    WorkStealQueue<T>::WorkStealQueue(int64_t c) {
        assert(c && (!(c & (c - 1))));
        _top.store(0, std::memory_order_relaxed);
        _bottom.store(0, std::memory_order_relaxed);
        _array.store(new AtomicArray<T>{c}, std::memory_order_relaxed);
        _garbage.reserve(32);
    }


    template<typename T>
    WorkStealQueue<T>::~WorkStealQueue() {
        for (auto a: _garbage) {
            delete a;
        }
        delete _array.load();
    }


    template<typename T>
    bool WorkStealQueue<T>::empty() const noexcept {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_relaxed);
        return b <= t;
    }


    template<typename T>
    size_t WorkStealQueue<T>::size() const noexcept {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_relaxed);
        return static_cast<size_t>(b >= t ? b - t : 0);
    }


    template<typename T>
    template<typename O>
    void WorkStealQueue<T>::push(O &&o) {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_acquire);
        AtomicArray<T> *a = _array.load(std::memory_order_relaxed);

        // queue is full
        if (a->capacity() - 1 < (b - t)) {
            AtomicArray<T> *tmp = a->resize(b, t);
            _garbage.push_back(a);
            std::swap(a, tmp);
            _array.store(a, std::memory_order_relaxed);
        }

        a->store(b, std::forward<O>(o));
        std::atomic_thread_fence(std::memory_order_release);
        _bottom.store(b + 1, std::memory_order_relaxed);
    }


    template<typename T>
    std::optional<T> WorkStealQueue<T>::pop() {
        int64_t b = _bottom.load(std::memory_order_relaxed) - 1;
        AtomicArray<T> *a = _array.load(std::memory_order_relaxed);
        _bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t t = _top.load(std::memory_order_relaxed);

        std::optional<T> item;

        if (t <= b) {
            item = a->load(b);
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


    template<typename T>
    std::optional<T> WorkStealQueue<T>::steal() {
        int64_t t = _top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t b = _bottom.load(std::memory_order_acquire);

        std::optional<T> item;

        if (t < b) {
            AtomicArray<T> *a = _array.load(std::memory_order_consume);
            item = a->load(t);
            if (!_top.compare_exchange_strong(t, t + 1,
                                              std::memory_order_seq_cst,
                                              std::memory_order_relaxed)) {
                return std::nullopt;
            }
        }

        return item;
    }


    template<typename T>
    int64_t WorkStealQueue<T>::capacity() const noexcept {
        return _array.load(std::memory_order_relaxed)->capacity();
    }
}

#endif //AR_WORKSTEALQUEUE_H
