#include "ar/coroutine.hpp"
#include "ar/iterator.hpp"
#include "ar/profiler.hpp"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;


void CoroutineHandler::Begin()
{
    if (Runtime::g_runtime != nullptr && Runtime::g_runtime->coroutine_counter) {
        Runtime::g_runtime->coroutine_counter->Increment();
    }

    PROFILER_ADD_EVENT(GetID(), Profiler::NEW_COROUTINE);
}


void CoroutineHandler::End()
{
    if (Runtime::g_runtime != nullptr && Runtime::g_runtime->coroutine_counter) {
        Runtime::g_runtime->coroutine_counter->Decrement();
    }

    PROFILER_ADD_EVENT(GetID(), Profiler::DELETE_COROUTINE);
}