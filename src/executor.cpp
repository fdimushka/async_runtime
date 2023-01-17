#include "ar/executor.hpp"
#include "ar/logger.hpp"
#include <utility>

using namespace AsyncRuntime;

Executor::Executor(std::string name_, std::vector<WorkGroupOption> work_groups_option, uint max_processors_count_) :
        name(std::move(name_)),
        processor_groups_option(std::move(work_groups_option)),
        max_processors_count(max_processors_count_)
{
    for (int i = 0; i < max_processors_count; ++i) {
        auto *processor = new Processor(i);
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