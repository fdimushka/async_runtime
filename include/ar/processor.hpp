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


namespace AsyncRuntime {
    class Executor;
    class ProcessorGroup;

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


        /**
         * @brief
         */
        void Run();


        /**
         * @brief
         */
        void Terminate();


        /**
         * @brief
         * @param task
         */
        void Post(Task *task);


        /**
         * @brief
         */
        void Notify();


        /**
         * @brief
         * @return
         */
        State GetState();


        /**
         * @brief
         * @return
         */
        [[nodiscard]] std::thread::id GetThreadId() const;
        bool IsSteal() const;
        bool IsSteal(ObjectID group_id) const;
        std::optional<Task*> Steal();
        std::optional<Task*> Steal(ObjectID group_id);
        std::vector<ProcessorGroup *> GetGroups();
        size_t GetGroupsSize();
        bool IsInGroup(const ProcessorGroup *group);

    protected:
        void Work();

        void ExecuteTask(Task* task, const ExecutorState &executor_state);
        void WaitTask();

        bool IsStealGlobal();
        std::optional<Task*> StealGlobal();
    private:
        CPU                                         cpu;
        std::vector<ProcessorGroup *>               groups;

        ThreadExecutor                              thread_executor;
        std::vector<int>                            rq_by_priority;
        std::vector<WorkStealQueue<Task*>>          local_run_queue;
        std::atomic_bool                            is_continue;
        std::atomic<State>                          state;

        std::condition_variable                     cv;
        std::mutex                                  mutex;
        std::mutex                                  group_mutex;

        std::atomic_int                             notify_count;
    };
}

#endif //AR_PROCESSOR_H
