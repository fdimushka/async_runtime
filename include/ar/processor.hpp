#ifndef AR_PROCESSOR_H
#define AR_PROCESSOR_H

#include "ar/object.hpp"
#include "ar/thread_executor.hpp"
#include "ar/work_steal_queue.hpp"
#include "ar/task.hpp"
#include "ar/notifier.hpp"
#include "ar/cpu_helper.hpp"

#include <map>
#include <set>
#include <thread>
#include <condition_variable>
#include <uv.h>

#include <oneapi/tbb.h>


namespace AsyncRuntime {
    class Executor;
    class ProcessorGroup;
#define MAX_GROUPS_COUNT                10

    /**
     * @class Processor
     * @brief
     */
    class Processor: public BaseObject {
    public:
        enum State {
            IDLE            =0,
            EXECUTE         =1,
            WAIT            =2,
        };

        explicit Processor(int pid, const CPU & cpu);
        ~Processor() override;

        Processor(Processor&&) = delete;
        Processor(const Processor&) = delete;

        Processor& operator =(const Processor&) = delete;
        Processor& operator =(Processor&&) = delete;

        void AddGroup(ProcessorGroup *group);

        void Run();

        void Terminate();

        void Post(const std::shared_ptr<task> & task);

        void Notify();

        State GetState();

        [[nodiscard]] std::thread::id GetThreadId() const;
        bool IsSteal() const;
        bool IsSteal(ObjectID group_id) const;
        std::shared_ptr<task> Pop();
        std::shared_ptr<task> Steal();
        std::shared_ptr<task> Steal(ObjectID group_id);
        std::vector<ProcessorGroup *> GetGroups();
        size_t GetGroupsSize();
        bool IsInGroup(const ProcessorGroup *group);

    protected:
        void Work();

        void ExecuteTask(const std::shared_ptr<task> & task, const task::execution_state &executor_state);
        void WaitTask();

        bool IsStealGlobal();
        std::shared_ptr<task> StealGlobal();
    private:
        CPU                                         cpu;
        std::vector<ProcessorGroup *>               groups;

        ThreadExecutor                              thread_executor;
        std::vector<int>                            rq_by_priority;
        oneapi::tbb::concurrent_queue<std::shared_ptr<task>>       local_run_queue[MAX_GROUPS_COUNT];
        std::atomic_bool                            is_continue;
        std::atomic<State>                          state;

        std::condition_variable                     cv;
        std::mutex                                  mutex;
        std::mutex                                  group_mutex;

        std::atomic_int                             notify_count;
    };
}

#endif //AR_PROCESSOR_H
