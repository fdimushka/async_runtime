#ifndef AR_DATAFLOW_FIFO_SHARED_BUFFER_H
#define AR_DATAFLOW_FIFO_SHARED_BUFFER_H

#include "ar/ar.hpp"
#include "ar/dataflow/shared_buffer.hpp"
#include "oneapi/tbb.h"

namespace AsyncRuntime::Dataflow {
    template< class T >
    class FIFOBuffer : public SharedBuffer< T > {
    public:
        explicit FIFOBuffer(size_t capacity = 0)
        : SharedBuffer<T>(kFIFO_BUFFER) {
            queue.set_capacity(capacity);
        };

        ~FIFOBuffer() override {
            Flush();
        };

        SharedBufferError Write( T && msg );
        SharedBufferError Write( const T & msg );
        std::optional<T> Read() override;
        bool TryRead( T & res ) override;
        bool Empty() override;
        void Flush() override;
    private:
        oneapi::tbb::concurrent_bounded_queue<T>        queue;
    };

    template<class T>
    SharedBufferError FIFOBuffer<T>::Write(T && msg) {
        if (queue.try_push(std::move(msg))) {
            return SharedBufferError::kNO_ERROR;
        } else {
            return SharedBufferError::kBUFFER_OVERFLOW;
        }
    }

    template<class T>
    SharedBufferError FIFOBuffer<T>::Write(const T & msg) {
        if (queue.try_push(msg)) {
            return SharedBufferError::kNO_ERROR;
        } else {
            return SharedBufferError::kBUFFER_OVERFLOW;
        }
    }

    template<class T>
    std::optional<T> FIFOBuffer<T>::Read() {
        T msg;
        if (queue.try_pop(msg)) {
            return msg;
        } else {
            return std::nullopt;
        }
    }

    template<class T>
    bool FIFOBuffer<T>::TryRead( T & res ) {
        return queue.try_pop(res);
    }

    template<class T>
    bool FIFOBuffer<T>::Empty() {
        return queue.empty();
    }

    template<class T>
    void FIFOBuffer<T>::Flush() {
        while (!queue.empty()) {
            T msg;
            if (!queue.try_pop(msg)) {
                break;
            }
        }
    }
}

#endif //AR_DATAFLOW_FIFO_SHARED_BUFFER_H
