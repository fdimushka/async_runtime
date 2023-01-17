#ifndef AR_PROCESSOR_GROUP_HPP
#define AR_PROCESSOR_GROUP_HPP

#include <iostream>
#include <map>
#include <atomic>
#include "ar/object.hpp"
#include "ar/task.hpp"
#include "ar/processor.hpp"
#include "ar/work_steal_queue.hpp"
#include "ar/thread_executor.hpp"
#include "ar/scheduler.hpp"

namespace AsyncRuntime {
#define WG_PRIORITY_LOW                 0
#define WG_PRIORITY_MEDIUM              1
#define WG_PRIORITY_HIGH                10
#define WG_PRIORITY_VERY_HIGH           20
#define MAX_GROUPS_COUNT                10

    class ProcessorGroup {
    public:
        struct GreaterThanByPriority
        {
            bool operator()(const ProcessorGroup* lhs, const ProcessorGroup* rhs) const {
                return lhs->priority < rhs->priority;
            }
        };

        ProcessorGroup(ObjectID id,
                       const std::vector<Processor*>& processor,
                       std::string  name, double util,  double cap, int priority = 0);
        ~ProcessorGroup() = default;

        void Post(Task *task);
        std::optional<Task *> Steal();
        std::optional<Task *> Steal(const ObjectID& processor_id);
        [[nodiscard]] bool IsSteal() const;
        [[nodiscard]] const std::string& GetName() const { return name; }
        [[nodiscard]] double GetCap() const { return cap; }
        [[nodiscard]] double GetUtil() const { return util; }
        [[nodiscard]] ObjectID GetID() const { return id; }
    private:
        std::vector<Processor*>         processors;
        std::shared_ptr<Scheduler>      scheduler;
        ObjectID                        id;
        double                          cap;
        double                          util;
        std::string                     name;
        int                             priority;
    };
}

#endif //AR_PROCESSOR_GROUP_HPP
