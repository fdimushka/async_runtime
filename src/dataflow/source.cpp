#include "ar/dataflow/source.hpp"

using namespace AsyncRuntime::Dataflow;

void Source::Flush() {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto it = Begin(); it != End(); it++) {
        it->second->Flush();
    }
}

void Source::Activate() {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto it = Begin(); it != End(); it++) {
        it->second->Activate();
    }
}

void Source::Deactivate() {

}