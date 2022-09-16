#include "ar/runtime.hpp"
#include "ar/io_executor.hpp"
#include "ar/logger.hpp"
#include "ar/profiler.hpp"

using namespace AsyncRuntime;


#define MAIN_EXECUTOR_NAME "main"
#define IO_EXECUTOR_NAME "io"

Runtime Runtime::g_runtime;


Runtime::Runtime() : main_executor{nullptr}, io_executor{nullptr}, is_setup(false)
{
    Logger::s_logger.SetStd();
}


Runtime::~Runtime()
{
    Terminate();
}


void Runtime::Setup(/*...*/)
{
    if(is_setup)
        return;

    CreateDefaultExecutors();
    is_setup = true;

    PROFILER_START();
}


void Runtime::Terminate()
{
    if(!is_setup)
        return;

    PROFILER_STOP();

    for(auto const& it : executors) {
        delete it.second;
    }

    executors.clear();

    is_setup = false;
}


void Runtime::CheckRuntime()
{
    assert(is_setup);
    assert(io_executor != nullptr);
    assert(main_executor != nullptr);
}


void Runtime::CreateDefaultExecutors()
{
    main_executor = CreateExecutor<Executor>(MAIN_EXECUTOR_NAME);
    io_executor = CreateExecutor<IOExecutor>(IO_EXECUTOR_NAME);

    for(const auto &processor : main_executor->GetProcessors()) {
        io_executor->ThreadRegistration(processor->GetThreadId());
    }

    io_executor->Run();
}


void Runtime::Post(Task *task)
{
    const auto& executor_state = task->GetDesirableExecutor();
    if(executor_state.executor == nullptr) {
        main_executor->Post(task);
    }else{
        executor_state.executor->Post(task);
    }
}