#include "ar/runtime.hpp"
#include "ar/logger.hpp"
#include "ar/profiler.hpp"
#include "ar/cpu_helper.hpp"
#include "ar/resource_pool.hpp"
#include "numbers.h"

#include "io_executor.h"

using namespace AsyncRuntime;

#define MAX_GROUPS_COUNT 10
#define MAIN_WORK_GROUP "main"
#define MAIN_EXECUTOR_NAME "main"
#define IO_EXECUTOR_NAME "io"

Runtime *Runtime::g_runtime;

Runtime::Runtime() : main_executor{nullptr}, io_executor{nullptr}, is_setup(false) {
}

Runtime::~Runtime() {
    Terminate();
}

void Runtime::Setup(const RuntimeOptions &_options) {
    Logger::s_logger.SetStd();

    SetupWorkGroups(_options.work_groups_option);

    if (is_setup)
        return;

    CreateDefaultExecutors(_options.virtual_numa_nodes_count);
    
    io_executor = CreateExecutor<IO::IOExecutor>(IO_EXECUTOR_NAME);

    is_setup = true;

    PROFILER_START();
}


void Runtime::Setup(Runtime *other) {
    g_runtime = other;
}


void Runtime::SetupWorkGroups(const std::vector<WorkGroupOption> &_work_groups_option) {
    work_groups_option.push_back({MAIN_WORK_GROUP, 1.0, 1.0, 0});

    if (work_groups_option.size() > MAX_GROUPS_COUNT)
        throw std::runtime_error("Work group size > " + std::to_string(MAX_GROUPS_COUNT));

    for (const auto &group: _work_groups_option) {
        if (group.name != MAIN_WORK_GROUP) {
            work_groups_option.push_back(group);
        } else {
            throw std::runtime_error("Work group \"" + group.name + "\" already exist!");
        }
    }
}


void Runtime::Terminate() {
    if (!is_setup)
        return;

    PROFILER_STOP();

    for (auto const &it: executors) {
        delete it.second;
    }

    executors.clear();

    is_setup = false;
}


void Runtime::CheckRuntime() {
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


void Runtime::CreateDefaultExecutors(int virtual_numa_nodes_count) {
    auto nodes = (virtual_numa_nodes_count == 0) ? GetNumaNodes() : GetManualNumaNodes(virtual_numa_nodes_count);
    std::vector<Executor *> cpu_executors;

    for (size_t i = 0; i < nodes.size(); ++i) {
        auto executor = new Executor("CPUExecutor_" + std::to_string(i), nodes[i].cpus, work_groups_option);
        executor->SetIndex(i);
        if (main_executor == nullptr) {
            main_executor = executor;
        }
        executors.insert(std::make_pair(i, executor));
        cpu_executors.push_back(executor);
    }

    if (main_executor == nullptr) {
        throw std::runtime_error("main executor not setup");
    }
}

ResourcePoolPtr Runtime::CreateResource(size_t chunk_sz, size_t nnext_size, size_t nmax_size) {
   return resources_manager.create_resource(chunk_sz, nnext_size, nmax_size);
}

void Runtime::DeleteResource(resource_pool *pool) {
    resources_manager.delete_resource(pool);
}

resource_pool * Runtime::GetDefaultResource() {
    return resources_manager.get_default_resource();
}

EntityTag Runtime::AddEntityTag(void *ptr) {
    auto executor = FetchFreeExecutor(kCPU_EXECUTOR);

    if (executor != nullptr) {
        int16_t entity_id = executor->AddEntity(ptr);
        int16_t executor_id = executor->GetIndex();
        return Numbers::Pack(executor_id, entity_id);
    } else {
        return INVALID_OBJECT_ID;
    }
}

void Runtime::DeleteEntityTag(EntityTag tag) {
    uint16_t executor_id, entity_id;
    Numbers::Unpack(tag, executor_id, entity_id);
    for (const auto &executor: executors) {
        if (executor.second->GetType() == kCPU_EXECUTOR && executor.second->GetIndex() == executor_id) {
            executor.second->DeleteEntity(entity_id);
            break;
        }
    }
}

IExecutor *Runtime::FetchExecutor(ExecutorType type, const EntityTag &tag) {
    uint16_t id, e;
    Numbers::Unpack(tag, id, e);
    for (const auto &executor: executors) {
        if (executor.second->GetType() == type && executor.second->GetIndex() == id) {
            return executor.second;
        }
    }
    return nullptr;
}

IExecutor *Runtime::FetchFreeExecutor(ExecutorType type) {
    IExecutor *min_executor = nullptr;
    int min = INT_MAX;

    for (auto &executor: executors) {
        if (executor.second->GetType() == type && min > executor.second->GetEntitiesCount()) {
            min = executor.second->GetEntitiesCount();
            min_executor = executor.second;
        }
    }

    return min_executor;
}

std::shared_ptr<Mon::Counter>
Runtime::MakeMetricsCounter(const std::string &name, const std::map<std::string, std::string> &labels) {
    if (metricer) {
        return metricer->MakeCounter(name, labels);
    } else {
        return {};
    }
}

void Runtime::CreateMetrics() {
    for (auto it : executors) {
        it.second->MakeMetrics(metricer);
    }
    coroutine_counter = MakeMetricsCounter("ar_coroutines_count", {});
}

void Runtime::Post(task *t) {
    const auto &executor_state = t->get_execution_state();
    if (executor_state.executor == nullptr) {
        auto executor = (executor_state.tag != INVALID_OBJECT_ID) ? FetchExecutor(kCPU_EXECUTOR, executor_state.tag) : FetchFreeExecutor(kCPU_EXECUTOR);
        if (executor != nullptr) {
            executor->Post(t);
        } else {
            main_executor->Post(t);
        }
    } else {
        executor_state.executor->Post(t);
    }
}