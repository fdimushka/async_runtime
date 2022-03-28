#ifndef AR_CHANNEL_H
#define AR_CHANNEL_H

#include "ar/object.hpp"
#include "ar/array.hpp"
#include "ar/task.hpp"


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
        bool Empty() const noexcept;


        /**
        @brief queries if the queue is full at the time of this call
        */
        bool Full() const noexcept;


        /**
        @brief queries the number of items at the time of this call
        */
        size_t Size() const noexcept;


        /**
         * @brief
         * @tparam O
         * @param item
         * @return
         */
        bool Send(T & item);


        /**
         * @brief
         * @tparam O
         * @return
         */
        std::shared_ptr<Result<T>> AsyncReceive();
    private:
        bool Push(T & item);


        std::optional<T> Pop();


        std::atomic<int64_t> _top;
        std::atomic<int64_t> _bottom;
        std::atomic<AtomicArray<T> *> _array;
        std::vector<AtomicArray<T> *> _garbage;
        std::shared_ptr<Result<T>>  result;
    };


    template<typename T>
    Channel<T>::Channel(int64_t c) : result(new Result<T>) {
        assert(c && (!(c & (c - 1))));
        _top.store(0, std::memory_order_relaxed);
        _bottom.store(0, std::memory_order_relaxed);
        _array.store(new AtomicArray<T>{c}, std::memory_order_relaxed);
        _garbage.reserve(32);
    }


    template<typename T>
    Channel<T>::~Channel() {
        for (auto a: _garbage) {
            delete a;
        }
        delete _array.load();
    }


    template<typename T>
    bool Channel<T>::Empty() const noexcept {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_relaxed);
        return b <= t;
    }


    template<typename T>
    bool Channel<T>::Full() const noexcept {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_acquire);
        AtomicArray<T> *a = _array.load(std::memory_order_relaxed);
        return a->capacity() - 1 < (b - t);
    }


    template<typename T>
    size_t Channel<T>::Size() const noexcept {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_relaxed);
        return static_cast<size_t>(b >= t ? b - t : 0);
    }


    template<typename T>
    bool Channel<T>::Push(T & o) {
        int64_t b = _bottom.load(std::memory_order_relaxed);
        int64_t t = _top.load(std::memory_order_acquire);
        AtomicArray<T> *a = _array.load(std::memory_order_relaxed);

        // queue is full
        if (a->capacity() - 1 < (b - t)) {
            return false;
        }

        a->store(b, o);
        std::atomic_thread_fence(std::memory_order_release);
        _bottom.store(b + 1, std::memory_order_relaxed);

        return true;
    }


    template<typename T>
    bool Channel<T>::Send(T & o) {
        return Push(o);
    }


    template<typename T>
    std::optional<T> Channel<T>::Pop() {
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
    std::shared_ptr<Result<T>> Channel<T>::AsyncReceive() {
        return std::shared_ptr<Result<T>>();
    }


    /**
     * @brief
     * @tparam T
     * @return
     */
    template <typename T>
    inline Channel<T> MakeChannel(size_t cap = 1024) {
        return Channel<T>(cap);
    }
}

#endif //AR_CHANNEL_H
