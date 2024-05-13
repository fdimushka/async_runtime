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
#include "ar/metricer.hpp"

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
                       std::string  name, const std::string & executor_name, double util,  double cap, int priority = 0);
        ~ProcessorGroup() = default;

        void Post(task *task);
        task *Steal();
        task *Steal(const ObjectID& processor_id);
        //[[nodiscard]] bool IsSteal() const;
        [[nodiscard]] const std::string& GetName() const { return name; }
        [[nodiscard]] double GetCap() const { return cap; }
        [[nodiscard]] double GetUtil() const { return util; }
        [[nodiscard]] ObjectID GetID() const { return id; }
        //const Scheduler *GetScheduler() const { return scheduler.get(); }
    private:
        void Notify();

        std::random_device rd;
        std::mt19937 gen;
        std::uniform_int_distribution<> distr;
        std::shared_ptr<Mon::Counter>   m_processors_count;
        std::vector<Processor*>         processors;
        std::unique_ptr<Scheduler>      scheduler;
        ObjectID                        id;
        double                          cap;
        double                          util;
        std::string                     name;
        int                             priority;
        TaskQueue<task*>                task_queue;
        std::mutex                      task_queue_mutex;
    };
}

#endif //AR_PROCESSOR_GROUP_HPP
