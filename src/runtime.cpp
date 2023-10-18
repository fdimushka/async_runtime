#include "ar/runtime.hpp"
#include "ar/io_executor.hpp"
#include "ar/logger.hpp"
#include "ar/profiler.hpp"

using namespace AsyncRuntime;


#define MAIN_WORK_GROUP "main"
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


void Runtime::Setup(const RuntimeOptions& _options)
{
    coroutine_counter = MakeMetricsCounter("coroutines_count", {"coroutines"});

    SetupWorkGroups(_options.work_groups_option);

    if(is_setup)
        return;

    CreateDefaultExecutors();
    is_setup = true;

    PROFILER_START();
}


void Runtime::SetupWorkGroups(const std::vector<WorkGroupOption>& _work_groups_option)
{
    work_groups_option.push_back({MAIN_WORK_GROUP,1.0,1.0, WG_PRIORITY_MEDIUM});

    if (work_groups_option.size() > MAX_GROUPS_COUNT)
        throw std::runtime_error("Work group size > " + std::to_string(MAX_GROUPS_COUNT));

    for(const auto &group : _work_groups_option) {
        if ( group.name != MAIN_WORK_GROUP) {
            work_groups_option.push_back(group);
        } else {
            throw std::runtime_error("Work group \"" + group.name + "\" already exist!");
        }
    }
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


ObjectID Runtime::GetWorkGroup(const std::string &name) const {
    for (size_t i = 0; i < work_groups_option.size(); ++i) {
        if (work_groups_option[i].name == name)
            return i;
    }
    return INVALID_OBJECT_ID;
}


void Runtime::CreateDefaultExecutors()
{
    main_executor = CreateExecutor<Executor>(MAIN_EXECUTOR_NAME, work_groups_option);
    io_executor = CreateExecutor<IOExecutor>(IO_EXECUTOR_NAME);

    for(const auto &processor : main_executor->GetProcessors()) {
        io_executor->ThreadRegistration(processor->GetThreadId());
    }

    io_executor->Run();
}

std::shared_ptr<Mon::Counter> Runtime::MakeMetricsCounter(const std::string & name, const std::vector<std::string> &tags) {
    if (metricer) {
        return metricer->MakeCounter(name, tags);
    } else {
        return {};
    }
}

void Runtime::Post(Task *task)
{
    const auto& executor_state = task->GetExecutorState();
    if(executor_state.executor == nullptr) {
        main_executor->Post(task);
    }else{
        executor_state.executor->Post(task);
    }
}