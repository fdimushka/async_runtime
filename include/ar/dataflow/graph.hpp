#ifndef AR_DATAFLOW_GRAPH_H
#define AR_DATAFLOW_GRAPH_H

#include "ar/ar.hpp"
#include "ar/dataflow/kernel.hpp"

namespace AsyncRuntime::Dataflow {
    /**
     * @class Graph
     * @brief
     */
    class Graph {
    public:
        void AddKernel();
    private:
        std::mutex  mutex;
    };
}

#endif //AR_DATAFLOW_GRAPH_H
