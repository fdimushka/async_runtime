#include "ar/dataflow/port.hpp"
#include "ar/dataflow/port_controller.hpp"

using namespace AsyncRuntime::Dataflow;

void PortController::AddUsedPort(const std::shared_ptr<Dataflow::Port> &port) {
    ports_used_.push_back(port);
}

void PortController::Subscribe(const PortUser *user) {
    for (const auto & port : ports_used_) {
        port->Subscribe(user);
    }
}

void PortController::Unsubscribe(const PortUser *user) {
    for (const auto & port : ports_used_) {
        port->Unsubscribe(user);
    }
}