#ifndef TEYE_STREAMSERVER_PORT_CONTROLLER_H
#define TEYE_STREAMSERVER_PORT_CONTROLLER_H

#include "ar/dataflow/port.hpp"
#include "ar/allocators.hpp"

namespace AsyncRuntime::Dataflow {

    class PortController {
        using PortPtrAllocator = Allocator<std::shared_ptr<Dataflow::Port>>;
    public:
        PortController() = default;
        PortController(resource_pool *resource) : ports_used_(PortPtrAllocator{resource}) {}
        ~PortController() = default;

        void AddUsedPort(const std::shared_ptr<Dataflow::Port> & port);
        void Subscribe(const PortUser *user);
        void Unsubscribe(const PortUser *user);
    private:
        std::vector<std::shared_ptr<Dataflow::Port>, PortPtrAllocator>    ports_used_;
    };
}

#endif //TEYE_STREAMSERVER_PORT_CONTROLLER_H
