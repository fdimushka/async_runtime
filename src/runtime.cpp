#include "ar/runtime.hpp"


using namespace AsyncRuntime;


#define MAIN_EXECUTOR_NAME "main"
#define IO_EXECUTOR_NAME "io"

Runtime Runtime::g_runtime;

Runtime::Runtime() : main_executor{nullptr}
{
}


Runtime::~Runtime()
{
}


void Runtime::Setup(/*...*/)
{
    CreateDefaultExecutors();
}


Executor* Runtime::CreateExecutor(const std::string& name, int max_processors_count)
{
    auto* wg = new Executor(name, max_processors_count);
    executors.insert(std::make_pair(wg->GetID(), wg));
    return wg;
}


void Runtime::CreateDefaultExecutors()
{
    main_executor = CreateExecutor(MAIN_EXECUTOR_NAME);
    //io_executor = CreateExecutor(IO_EXECUTOR_NAME, 1);
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


void Runtime::Post(Executor *executor, Task *task)
{
    assert(executor != nullptr);

    executor->Post(task);
}