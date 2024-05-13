#include "ar/processor_group.hpp"
#include "ar/runtime.hpp"
#include "numbers.h"

#include <utility>
#include "ar/processor.hpp"

using namespace AsyncRuntime;

ProcessorGroup::ProcessorGroup(ObjectID _id,
                               const std::vector<Processor*>& all_processor,
                               std::string _name,
                               const std::string & executor_name,
                               double _util,
                               double _cap,
                               int _priority) :
        priority(_priority)
        , name(std::move(_name))
        , cap(_cap)
        , gen(rd())
        , util(_util)
        , id(_id)
{
    int processors_count = (int) ((double) all_processor.size() * (1.0 / (cap / util)));
    processors_count = std::min(std::max(2, processors_count), (int)all_processor.size());
    int current_prc = 0;

    while (processors.size() < processors_count) {
        auto it = std::min_element(all_processor.begin() + current_prc,
                                   all_processor.end(),
                                   [](Processor *first, Processor *second) {
                                       return first->GetGroupsSize() < second->GetGroupsSize();
                                   });

        if (it != all_processor.end()) {
            auto processor = *it;
            if (!processor->IsInGroup(this)) {
                processor->AddGroup(this);
                processors.push_back(processor);
            }
        }

        current_prc = (current_prc + 1) % processors_count;
    }

    m_processors_count = Runtime::g_runtime->MakeMetricsCounter("processors_count", {
            {"work_group", name}, {"executor", executor_name}
    });

    if (m_processors_count) {
        m_processors_count->Increment(static_cast<double>(processors.size()));
    }

    distr = std::uniform_int_distribution<>(0, processors.size() - 1);
    scheduler = std::make_unique<Scheduler>([this](task *task) {
        Post(task);
    });
}


void ProcessorGroup::Post(task *task)
{
    task->set_execution_state_wg(GetID());
    if (task->get_delay() <= 0) {
        auto &state = task->get_execution_state();
        if (state.processor != INVALID_OBJECT_ID) {
            bool posted = false;
            for (auto & processor : processors) {
                if (processor->GetID() == state.processor) {
                    processor->Post(task);
                    posted = true;
                }
            }

            if (!posted) {
                if (state.tag != INVALID_OBJECT_ID) {
                    uint16_t executor_index;
                    uint16_t entity_index;
                    Numbers::Unpack(state.tag, executor_index, entity_index);
                    int pid = (int)entity_index % processors.size();
                    std::cout << pid << std::endl;
                    processors[pid]->Post(task);
                } else {
                    std::lock_guard<std::mutex> lock(task_queue_mutex);
                    task_queue.push(task, static_cast<unsigned int>(TaskPriority::NORMAL));
                    Notify();
                }
            }
        } else {
            if (state.tag != INVALID_OBJECT_ID) {
                uint16_t executor_index;
                uint16_t entity_index;
                Numbers::Unpack(state.tag, executor_index, entity_index);
                int pid = (int)entity_index % processors.size();
                std::cout << pid << std::endl;
                processors[pid]->Post(task);
            } else {
                std::lock_guard<std::mutex> lock(task_queue_mutex);
                task_queue.push(task, static_cast<unsigned int>(TaskPriority::NORMAL));
                Notify();
            }
        }
    } else {
        scheduler->Post(task);
    }
}


task *ProcessorGroup::Steal()
{
    auto task = task_queue.steal();
    if (!task) {
        for (auto processor : processors) {
            task = processor->Steal(GetID());
            if (task) {
                return task;
            }
        }
    }
    return task;
}


task *ProcessorGroup::Steal(const ObjectID &processor_id)
{
    auto task = task_queue.steal();
    if (!task) {
        for (auto processor : processors) {
            if (processor->GetID() != processor_id) {
                task = processor->Steal(GetID());
                if (task) {
                    return task;
                }
            }
        }
    }
    return task;
}

void ProcessorGroup::Notify() {
    Processor *p = nullptr;
    int min_tasks = INT_MAX;
    for (auto processor : processors) {
        if (processor->state.load(std::memory_order_relaxed) == Processor::WAIT) {
            processor->Notify();
            return;
        }

        int n = processor->notify_count.load(std::memory_order_relaxed);
        if (n < min_tasks) {
            min_tasks = n;
            p = processor;
        }
    }
    assert(p);
    p->Notify();
}