#ifndef AR_DATAFLOW_SHARED_BUFFER_H
#define AR_DATAFLOW_SHARED_BUFFER_H

#include "ar/ar.hpp"
#include "ar/metricer.hpp"
#include <typeinfo>

namespace AsyncRuntime::Dataflow {
    enum SharedBufferType {
        kFIFO_BUFFER,
        kTRIPLE_BUFFER
    };

    enum class SharedBufferError {
        kNO_ERROR           = 0,
        kBUFFER_OVERFLOW    = 1,
        kBUFFER_NOT_FOUND   = 2
    };

    template< class T >
    class FIFOBuffer;

    template< class T >
    class TripleBuffer;

    template< class T >
    class SharedBuffer {
    public:
        explicit SharedBuffer(SharedBufferType type) : type(type) {};
        virtual ~SharedBuffer() = default;
        virtual SharedBufferError Write(const T & msg) = 0;
        virtual SharedBufferError Write(T && msg) = 0;
        virtual std::optional<T> Read() = 0;
        virtual bool TryRead(T & res) = 0;
        virtual bool Empty() =0;
        virtual void Flush() = 0;
        void SetSkipCounter(const std::shared_ptr<Mon::Counter> & counter) { skip_counter = counter; }
        const SharedBufferType GetType() const { return type; }
    protected:
        std::shared_ptr<Mon::Counter> skip_counter;
    private:
        SharedBufferType type;
    };
}

#endif //AR_DATAFLOW_SHARED_BUFFER_H
