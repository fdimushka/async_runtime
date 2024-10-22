#ifndef AR_DATAFLOW_FIFO_SHARED_BUFFER_H
#define AR_DATAFLOW_FIFO_SHARED_BUFFER_H

#include "ar/ar.hpp"
#include "ar/dataflow/shared_buffer.hpp"
#include "config.hpp"

#include <boost/lockfree/queue.hpp>

namespace AsyncRuntime::Dataflow {
    template< class T >
    class FIFOBuffer : public SharedBuffer< T > {
    public:
        explicit FIFOBuffer(size_t cap = 1000): SharedBuffer<T>(kFIFO_BUFFER), capacity(cap) {
        };

        ~FIFOBuffer() override {
            Flush();
        };

        SharedBufferError Write(T && msg) override {
            std::lock_guard<std::mutex> lock(mutex);
            if (queue.size() < capacity) {
                queue.push(std::move(msg));
                return SharedBufferError::kNO_ERROR;
            } else {
                if (this->skip_counter) {
                    this->skip_counter->Increment();
                }
                return SharedBufferError::kBUFFER_OVERFLOW;
            }
        }

        SharedBufferError Write(const T & msg) override {
            std::lock_guard<std::mutex> lock(mutex);
            if (queue.size() < capacity) {
                queue.push(msg);
                return SharedBufferError::kNO_ERROR;
            } else {
                if (this->skip_counter) {
                    this->skip_counter->Increment();
                }
                return SharedBufferError::kBUFFER_OVERFLOW;
            }
        }

        std::optional<T> Read() override {
            std::lock_guard<std::mutex> lock(mutex);
            if (!queue.empty()) {
                T msg = queue.front();
                queue.pop();
                return msg;
            } else {
                return std::nullopt;
            }
        }

        bool TryRead( T & res ) override {
            std::lock_guard<std::mutex> lock(mutex);
            if (!queue.empty()) {
                res = queue.front();
                queue.pop();
                return true;
            } else {
                return false;
            }
        }

        bool Empty() override {
            std::lock_guard<std::mutex> lock(mutex);
            return queue.empty();
        }

        void Flush() override {
            std::lock_guard<std::mutex> lock(mutex);
            while (!queue.empty()) {
                queue.pop();
            }
        }
    private:
        int capacity;
        std::mutex mutex;
        std::queue<T> queue;
    };
}

#endif //AR_DATAFLOW_FIFO_SHARED_BUFFER_H
