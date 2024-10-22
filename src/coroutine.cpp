#include "ar/coroutine.hpp"
#include "ar/profiler.hpp"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;

void coroutine_handler::create() {
    if (Runtime::g_runtime != nullptr && Runtime::g_runtime->coroutine_counter) {
        Runtime::g_runtime->coroutine_counter->Increment();
    }

    PROFILER_ADD_EVENT(reinterpret_cast<std::uintptr_t>(this), Profiler::NEW_COROUTINE);
}

void coroutine_handler::destroy() {
    if (Runtime::g_runtime != nullptr && Runtime::g_runtime->coroutine_counter) {
        Runtime::g_runtime->coroutine_counter->Decrement();
    }

    PROFILER_ADD_EVENT(reinterpret_cast<std::uintptr_t>(this), Profiler::DELETE_COROUTINE);
}

void AsyncRuntime::set_current_resource(resource_pool *resource) {
    if (Runtime::g_runtime != nullptr) {
        Runtime::g_runtime->SetCurrentResource(resource);
    }
}
