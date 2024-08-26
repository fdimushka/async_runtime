#ifndef AR_DATAFLOW_H
#define AR_DATAFLOW_H

#include "ar/dataflow/sink.hpp"
#include "ar/dataflow/source.hpp"
#include "ar/dataflow/kernel.hpp"
#include "ar/dataflow/graph.hpp"
#include "ar/dataflow/port_controller.hpp"

namespace AsyncRuntime::Dataflow {
    template<class T>
    inline void Connect(const std::shared_ptr<Dataflow::SinkPort<T>> &sink,
                        const std::shared_ptr<Dataflow::SourcePort<T>> &source) { sink->Connect(source); }

    template<class T>
    inline void Connect(const std::shared_ptr<Dataflow::SinkPort<T>> &sink,
                        const std::shared_ptr<Dataflow::Consumer<T>> &consumer) { sink->Connect(consumer); }

    template<class T>
    inline void Disconnect(const std::shared_ptr<Dataflow::SinkPort<T>> &sink,
                           const std::shared_ptr<Dataflow::SourcePort<T>> &source) { sink->Disconnect(source); }

    template<class T>
    inline void Disconnect(const std::shared_ptr<Dataflow::SinkPort<T>> &sink,
                           const std::shared_ptr<Dataflow::Consumer<T>> &consumer) { sink->Disconnect(consumer); }
}

#endif //AR_DATAFLOW_H
