#include "ar/processor_group.hpp"
#include "ar/runtime.hpp"

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
        , util(_util)
        , id(_id)
{
    int processors_count = (int) ((double) all_processor.size() * (1.0 / (cap / util)));
    processors_count = std::min(std::max(1, processors_count), (int)all_processor.size());
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

    m_processors_count = Runtime::g_runtime.MakeMetricsCounter("processors_count", {
            {"work_group", name}, {"executor", executor_name}
    });

    if (m_processors_count) {
        m_processors_count->Increment(static_cast<double>(processors.size()));
    }

    scheduler = std::make_shared<Scheduler>(processors);
}


void ProcessorGroup::Post(Task *task)
{
    if( scheduler ) {
        task->SetWorkGroupExecutorState(GetID());
        scheduler->Post(task);
    }
}


std::optional<Task *> ProcessorGroup::Steal()
{
    if( scheduler ) {
        auto task = scheduler->Steal();
        if (!task) {
            for (int i = 0; i < processors.size() && !task; i++) {
                auto processor = processors[i];
                if (processor->GetState() == Processor::EXECUTE &&
                    processor->IsSteal(GetID())) {
                    task = processor->Steal(GetID());
                }
            }
        }
        return task;
    } else {
        return std::nullopt;
    }
}


std::optional<Task *> ProcessorGroup::Steal(const ObjectID &processor_id)
{
    if( scheduler ) {
        auto task = scheduler->Steal();
        if (!task) {
            for (int i = 0; i < processors.size() && !task; i++) {
                auto processor = processors[i];
                if (processor->GetID() != processor_id &&
                    processor->GetState() == Processor::EXECUTE &&
                    processor->IsSteal(GetID())) {
                    task = processor->Steal(GetID());
                }
            }
        }
        return task;
    } else {
        return std::nullopt;
    }
}


bool ProcessorGroup::IsSteal() const
{
    if( scheduler ) {
        if (!scheduler->IsSteal()) {
            return std::any_of(processors.begin(),
                               processors.end(),
                               [](const Processor *processor) { return processor->IsSteal(); });
        } else {
            return true;
        }
    } else {
        return false;
    }
}