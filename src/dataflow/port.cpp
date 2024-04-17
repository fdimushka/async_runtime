#include "ar/dataflow/port.hpp"

using namespace AsyncRuntime::Dataflow;

Port::Port(const std::string & name, size_t data_type)
    : name(name), data_type(data_type) {
}

void Port::Subscribe(const PortUser *user) {
    subscribers.insert(user);
    subscribers_count.store(subscribers.size(), std::memory_order_relaxed);
}

void Port::Unsubscribe(const PortUser *user) {
    subscribers.erase(user);
    subscribers_count.store(subscribers.size(), std::memory_order_relaxed);
}

bool Port::SubscribersEmpty() {
    return subscribers_count.load(std::memory_order_relaxed) == 0;
}

void Port::UnsubscribeAll() {
    subscribers.clear();
    subscribers_count.store(0, std::memory_order_relaxed);
}
