#ifndef AR_DATAFLOW_CONSUMER_H
#define AR_DATAFLOW_CONSUMER_H

#include "ar/ar.hpp"
#include "ar/dataflow/port.hpp"
#include <typeinfo>

namespace AsyncRuntime::Dataflow {

    /**
     * @class Consumer
     * @tparam T
     */
    template<typename T>
    class Consumer : public Port {
    public:
        Consumer(const std::string & name, size_t data_type) : Port(name, data_type) { };
        virtual ~Consumer() override = default;
        using element_type = T;

        virtual bool IsActive() = 0;

        virtual int Write(T && msg) = 0;

        virtual int Write(const T & msg) = 0;
    };

    /**
     * @brief
     * @tparam T
     */
    class RawConsumer {
    public:
        virtual ~RawConsumer() = default;
        virtual int Write(const uint8_t *buffer, size_t size) = 0;
        virtual bool IsActive() { return true; };
    };
}
#endif //AR_DATAFLOW_CONSUMER_H
