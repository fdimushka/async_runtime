#ifndef TEYE_STREAMSERVER_PORT_CONTROLLER_H
#define TEYE_STREAMSERVER_PORT_CONTROLLER_H

#include "ar/dataflow/port.hpp"

namespace AsyncRuntime::Dataflow {

    class PortController {
    public:
        PortController() = default;
        ~PortController() = default;

        void AddUsedPort(const std::shared_ptr<Dataflow::Port> & port);
        void Subscribe(const PortUser *user);
        void Unsubscribe(const PortUser *user);
    private:
        std::vector<std::shared_ptr<Dataflow::Port>>    ports_used_;
    };
}

#endif //TEYE_STREAMSERVER_PORT_CONTROLLER_H
