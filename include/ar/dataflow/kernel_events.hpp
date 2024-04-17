#ifndef AR_DATAFLOW_KERNEL_EVENTS_H
#define AR_DATAFLOW_KERNEL_EVENTS_H


namespace AsyncRuntime::Dataflow {

    /**
     * @enum KernelEvent
     * @brief
     */
    enum class KernelEvent
    {
        kKERNEL_EVENT_ANY                       = 1 << 1, // 1
        kKERNEL_EVENT_READ_SOURCE               = 1 << 1, // 1
        kKERNEL_EVENT_TERMINATE                 = 1 << 2, // 2
        kKERNEL_EVENT_SINK_SUBSCRIPTION         = 1 << 3, // 4
        kKERNEL_EVENT_SINK_UNSUBSCRIPTION       = 1 << 4, // 8
//        Flag5 = 1 << 4, // 16
//        Flag6 = 1 << 5, // 32
//        Flag7 = 1 << 6, // 64
//        Flag8 = 1 << 7  //128
    };
}

#endif //AR_DATAFLOW_KERNEL_EVENTS_H
