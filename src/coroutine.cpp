#include "ar/coroutine.hpp"
#include "ar/iterator.hpp"
#include "ar/profiler.hpp"

using namespace AsyncRuntime;


void CoroutineHandler::Begin()
{
    PROFILER_ADD_EVENT(GetID(), Profiler::NEW_COROUTINE);
}


void CoroutineHandler::End()
{
    PROFILER_ADD_EVENT(GetID(), Profiler::DELETE_COROUTINE);
}