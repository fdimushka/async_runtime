#include "ar/executor.hpp"
#include "ar/logger.hpp"
#include "ar/runtime.hpp"
#include <utility>

using namespace AsyncRuntime;


IExecutor::IExecutor(const std::string & name, ExecutorType executor_type) : type(executor_type) {
    m_entities_count = Runtime::g_runtime.MakeMetricsCounter("entities_count", {
            {"executor", name}
    });

    if (m_entities_count) {
        m_entities_count->Increment(0);
    }
}


void IExecutor::IncrementEntitiesCount() {
    if (m_entities_count) {
        m_entities_count->Increment();
    }

    entities_count.fetch_add(1, std::memory_order_relaxed);
}


void IExecutor::DecrementEntitiesCount() {
    if (m_entities_count) {
        m_entities_count->Decrement();
    }

    if (entities_count.fetch_sub(1, std::memory_order_relaxed) <= 0) {
        entities_count.store(0, std::memory_order_relaxed);
    }
}


Executor::Executor(std::string  name_,
                   const std::vector<AsyncRuntime::CPU> & cpus,
                   std::vector<WorkGroupOption> work_groups_option) :
        IExecutor(name_, kCPU_EXECUTOR),
        name(std::move(name_)),
        processor_groups_option(std::move(work_groups_option)),
        max_processors_count(cpus.size())
{
    for (const auto & cpu : cpus) {
        auto *processor = new Processor(cpu);
        processors.push_back(processor);
    }

    for (int i = 0; i < processor_groups_option.size(); ++i) {
        const auto &option = processor_groups_option[i];
        auto *group = new ProcessorGroup(i, processors, option.name, option.util, option.cap, option.priority);
        processor_groups.push_back(group);
    }

    main_processor_group = processor_groups[0];

    for (auto processor: processors) {
        processor->Run();
    }

    processors_count = Runtime::g_runtime.MakeMetricsCounter("processors_count", {
            {"executor", name}
    });

    if (processors_count) {
        processors_count->Increment(static_cast<double>(processors.size()));
    }
}


Executor::~Executor() {
    for (auto processor: processors) {
        processor->Terminate();
        delete processor;
    }

    processors.clear();

    for (auto group: processor_groups) {
        delete group;
    }

    processor_groups.clear();
}

void Executor::Post(Task *task) {
    const auto &execute_state = task->GetExecutorState();
    task->SetExecutorExecutorState(this);

    if (task->GetDelay() <= 0 &&
        execute_state.processor != INVALID_OBJECT_ID &&
        execute_state.processor >= 0 &&
        execute_state.processor < processors.size())
    {
        processors[execute_state.processor]->Post(task);
    } else if ( execute_state.work_group != INVALID_OBJECT_ID &&
                execute_state.work_group >= 0 &&
                execute_state.work_group < processor_groups.size())
    {
        processor_groups[execute_state.work_group]->Post(task);
    } else {
        main_processor_group->Post(task);
    }
}