#include "ar/runtime.hpp"
#include "ar/io_executor.hpp"
#include "ar/logger.hpp"

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
}


void Runtime::Terminate()
{
    if(!is_setup)
        return;

    if(io_executor != nullptr) {
        delete io_executor;
        io_executor = nullptr;
    }

    if(main_executor != nullptr) {
        delete main_executor;
        main_executor = nullptr;
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
    main_executor = new Executor(MAIN_EXECUTOR_NAME);
    io_executor = new IOExecutor(IO_EXECUTOR_NAME);

    for(const auto &processor : main_executor->GetProcessors()) {
        io_executor->ThreadRegistration(processor->GetThreadId());
    }

    io_executor->Run();

    executors.insert(std::make_pair(main_executor->GetID(), main_executor));
    executors.insert(std::make_pair(io_executor->GetID(), io_executor));
}


void Runtime::Post(Task *task)
{
    assert(task != nullptr);

    const auto& executor_state = task->GetDesirableExecutor();
    if(executor_state.executor == nullptr) {
        Post(main_executor, task);
    }else{
        Post(executor_state.executor, task);
    }
}


void Runtime::Post(IExecutor *executor, Task *task)
{
    assert(executor != nullptr);
    executor->Post(task);
}