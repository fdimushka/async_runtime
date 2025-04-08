#include "ar/coroutine.hpp"
#include "ar/profiler.hpp"
#include "ar/runtime.hpp"

#define BOOST_CHRONO_HEADER_ONLY
#include <boost/chrono/thread_clock.hpp>

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
