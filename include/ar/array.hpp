#ifndef AR_ARRAY_H
#define AR_ARRAY_H

#include <atomic>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <optional>


namespace AsyncRuntime {


    /**
     * @struct Array
     * @brief Lock-free array
     */
    template<typename T>
    struct AtomicArray {
        int64_t C;
        int64_t M;
        std::atomic_size_t U;
        std::atomic<T> *S;


        explicit AtomicArray(int64_t c) :
                C{c},
                M{c - 1},
                U{0},
                S{new std::atomic<T>[static_cast<size_t>(C)]} {
        }


        ~AtomicArray() {
            delete[] S;
        }


        [[nodiscard]] int64_t capacity() const noexcept {
            return C;
        }


        [[nodiscard]] size_t size() const noexcept {
            return U.load(std::memory_order_relaxed);
        }


        template<typename O>
        void store(int64_t i, O &&o) {
            S[i & M].store(std::forward<O>(o), std::memory_order_relaxed);
            U.fetch_add(1, std::memory_order_relaxed);
        }


        T load(int64_t i) noexcept {
            return S[i & M].load(std::memory_order_relaxed);
        }


        T operator [](int64_t i) noexcept {
            return S[i & M].load(std::memory_order_relaxed);
        }


        AtomicArray *resize(int64_t b, int64_t t) {
            auto *ptr = new AtomicArray{2 * C};
            for (int64_t i = t; i != b; ++i) {
                ptr->store(i, load(i));
            }
            return ptr;
        }
    };
}

#endif //AR_ARRAY_H
