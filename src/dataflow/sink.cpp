#include "ar/dataflow/sink.hpp"

using namespace AsyncRuntime::Dataflow;

std::shared_ptr<Port> Sink::operator[](const std::string &port_name) {
    return At(port_name);
}

std::shared_ptr<Port> Sink::At(const std::string &port_name) {
    const auto &it = port_map.find(port_name);
    if (it == port_map.cend()) {
        throw std::runtime_error("Source port not found");
    }
    return it->second;
}

void Sink::Subscribe(const std::string &port_name, const PortUser *user) {
    std::lock_guard<std::mutex> lock(mutex);
    At(port_name)->Subscribe(user);
}

void Sink::Unsubscribe(const std::string &port_name, const PortUser *user) {
    std::lock_guard<std::mutex> lock(mutex);
    At(port_name)->Unsubscribe(user);
}


void Sink::UnsubscribeAll() {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto it = Begin(); it != End(); it++) {
        it->second->UnsubscribeAll();
    }
}

void Sink::DisconnectAll() {
    for (auto it = Begin(); it != End(); it++) {
        //it->second->
    }
}

bool Sink::SubscribersEmpty() {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto it = Begin(); it != End(); it++) {
        if (!it->second->SubscribersEmpty()) { return false; }
    }
    return true;
}