#ifndef AR_PROCESSOR_H
#define AR_PROCESSOR_H

#include "ar/object.hpp"
#include "ar/thread_executor.hpp"
#include "ar/work_steal_queue.hpp"
#include "ar/task.hpp"
#include "ar/notifier.hpp"


#include <map>
#include <set>
#include <thread>
#include <condition_variable>

#include <uv.h>


namespace AsyncRuntime {
    class Executor;


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

        explicit Processor(Executor* executor_ = nullptr);
        ~Processor() override;


        Processor(Processor&&) = delete;
        Processor(const Processor&) = delete;

        Processor& operator =(const Processor&) = delete;
        Processor& operator =(Processor&&) = delete;


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
    protected:
        void Work();
        void ExecuteTask(Task* task);
        void WaitTask();


        std::optional<Task*> ConsumeWork();
        std::optional<Task*> StealWorkGlobal();
        std::optional<Task*> StealWorkLocal();
        std::optional<Task*> StealWorkOnOthers();
    private:
        ThreadExecutor                              thread_executor;
        WorkStealQueue<Task*>                       local_run_queue;
        std::atomic_bool                            is_continue;
        std::atomic<State>                          state;

        Executor*                                   executor;
        ExecutorState                               executor_state;

        std::condition_variable                     cv;
        std::mutex                                  mutex;

        std::atomic_int                             notify_count;
    };
}

#endif //AR_PROCESSOR_H
