#include "ar/executor.hpp"
#include "ar/logger.hpp"
#include "ar/runtime.hpp"
#include <utility>

using namespace AsyncRuntime;

IExecutor::IExecutor(const std::string & ex_name, ExecutorType executor_type) : name(ex_name), type(executor_type) {
    m_entities_count = Runtime::g_runtime->MakeMetricsCounter("entities_count", {
            {"executor", name}
    });

    if (m_entities_count) {
        m_entities_count->Increment(0);
    }
}

uint16_t IExecutor::AddEntity(void *ptr) {
    if (m_entities_count) {
        m_entities_count->Increment();
    }

    entities_count.fetch_add(1, std::memory_order_relaxed);
    
    return 0;
}


void IExecutor::DeleteEntity(uint16_t id) {
    if (m_entities_count) {
        m_entities_count->Decrement();
    }

    if (entities_count.fetch_sub(1, std::memory_order_relaxed) <= 0) {
        entities_count.store(0, std::memory_order_relaxed);
    }
}


Executor::Executor(const std::string & name_,
                   const std::vector<AsyncRuntime::CPU> & cpus,
                   std::vector<WorkGroupOption> work_groups_option) :
        IExecutor(name_, kCPU_EXECUTOR),
        processor_groups_option(std::move(work_groups_option)),
        max_processors_count(cpus.size())
{
    for (int i =0; i < cpus.size(); ++i) {
        auto *processor = new Processor(i, cpus[i]);
        processors.push_back(processor);
    }

    for (int i = 0; i < processor_groups_option.size(); ++i) {
        const auto &option = processor_groups_option[i];
        auto *group = new ProcessorGroup(i, processors, option.name, name, option.util, option.cap, option.priority);
        processor_groups.push_back(group);
    }

    main_processor_group = processor_groups[0];

    for (auto processor: processors) {
        processor->Run();
    }

    processors_count = Runtime::g_runtime->MakeMetricsCounter("processors_count", {
            {"executor", name}
    });

    if (processors_count) {
        processors_count->Increment(static_cast<double>(processors.size()));
    }
}


Executor::~Executor() {
    for (auto processor: processors) {
        processor->Terminate();
    }

    processors.clear();

    for (auto group: processor_groups) {
        delete group;
    }

    for (auto processor: processors) {
        delete processor;
    }

    processor_groups.clear();
}

std::vector<std::thread::id> Executor::GetThreadIds() {
    std::vector<std::thread::id> ids;
    for (const auto & p : processors) {
        ids.push_back(p->GetThreadId());
    }

    for (const auto group: processor_groups) {
        ids.push_back(group->GetScheduler()->GetThreadId());
    }

    return ids;
}

void Executor::Post(Task *task) {
    const auto &execute_state = task->GetExecutorState();
    task->SetExecutorExecutorState(this);

    if (task->GetDelay() <= 0) {
        if (execute_state.processor != INVALID_OBJECT_ID) {
            processors[execute_state.processor]->Post(task);
        } else if (execute_state.work_group != INVALID_OBJECT_ID) {
            processor_groups[execute_state.work_group]->Post(task);
        } else {
            main_processor_group->Post(task);
        }
    } else {
        if (execute_state.work_group != INVALID_OBJECT_ID) {
            processor_groups[execute_state.work_group]->Post(task);
        } else {
            main_processor_group->Post(task);
        }
    }
}