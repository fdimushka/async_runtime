#ifndef AR_DATAFLOW_SHARED_BUFFER_FACTORY_H
#define AR_DATAFLOW_SHARED_BUFFER_FACTORY_H

#include "ar/ar.hpp"
#include "ar/dataflow/shared_buffer.hpp"
#include "ar/dataflow/triple_shared_buffer.hpp"
#include "ar/dataflow/fifo_shared_buffer.hpp"

namespace AsyncRuntime::Dataflow {

    class SharedBufferFactory {
    public:

        template< class T, class ...Arguments >
        static SharedBuffer<T> *Make(SharedBufferType type, Arguments &&... args);
    };

    template<class T, class... Arguments>
    SharedBuffer<T> *SharedBufferFactory::Make(SharedBufferType type, Arguments &&... args) {
        if (type == kFIFO_BUFFER) {
            return new FIFOBuffer<T>(std::forward<Arguments>(args)...);
        } else {
            return new TripleBuffer<T>();
        }
    }
}


#endif //AR_DATAFLOW_SHARED_BUFFER_FACTORY_H
