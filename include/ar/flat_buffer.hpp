#ifndef AR_FLAT_BUFFER_H
#define AR_FLAT_BUFFER_H

#include "ar/allocators.hpp"

namespace AsyncRuntime {

    class coroutine_handler;

    template<typename T, typename Alloc = Allocator<T>>
    class flat_buffer {
    public:
        flat_buffer() = default;
        explicit flat_buffer(std::size_t sz) : allocator{}, size(sz) {
            begin = allocator.allocate(size*sizeof(T));
        }

        flat_buffer(const Alloc & alloc, std::size_t sz) : allocator(alloc), size(sz) {
            begin = allocator.allocate(size*sizeof(T));
        }

        flat_buffer(const coroutine_handler *handler, std::size_t sz) : allocator(handler), size(sz) {
            begin = allocator.allocate(size*sizeof(T));
        }

        flat_buffer(resource_pool *resource, std::size_t sz) : allocator(resource), size(sz) {
            begin = allocator.allocate(size*sizeof(T));
        }

        ~flat_buffer() { free(); }

        const T *get_data() const { return begin; }

        T *get_data() { return begin; }

        size_t get_size() const { return size; }

        void free() {
            if (size != 0) {
                allocator.deallocate(begin, size*sizeof(T));
                size = 0;
            }
        }
    private:
        T *begin = nullptr;
        size_t size = 0;
        Alloc allocator;
    };
}

#endif //AR_FLAT_BUFFER_H
