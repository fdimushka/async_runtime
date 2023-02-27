#ifndef AR_EXECUTOR_H
#define AR_EXECUTOR_H

#include "ar/processor.hpp"

#ifdef USE_TESTS
class EXECUTOR_TEST_FRIEND;
#endif


namespace AsyncRuntime {

    class IExecutor: public BaseObject {
    public:
        virtual void Post(Task* task) = 0;
    };


    class Executor  : public IExecutor {
        friend Processor;
#ifdef USE_TESTS
        friend EXECUTOR_TEST_FRIEND;
#endif
    public:
        typedef std::priority_queue<Task*, std::vector<Task*>, Task::LessThanByDelay> TasksPq;

        explicit Executor(std::string  name_,
                           uint max_processors_count_ = std::thread::hardware_concurrency());
        ~Executor() override;


        Executor(const Executor&) = delete;
        Executor(Executor&&) = delete;


        Executor& operator =(const Executor&) = delete;
        Executor& operator =(Executor&&) = delete;


        /**
         * @brief
         * @param task
         */
        void Post(Task* task) override;


        /**
         * @brief
         * @return
         */
        const std::vector<Processor*>& GetProcessors() const { return processors; }
    private:
        void SchedulerLoop();
        void Spawn();
        void RunQueuePush(Task *task);


        ThreadExecutor              scheduler_executor;
        uint                        max_processors_count;
        WorkStealQueue<Task*>       run_queue;
        TasksPq                     delayed_task;
        std::condition_variable     delayed_task_cv;
        std::mutex                  delayed_task_mutex;
        std::mutex                  run_queue_mutex;
        std::vector<Processor*>     processors;
        std::string                 name;
        size_t                      notify_inc;
        std::atomic_bool            is_continue;
    };
}

#endif //AR_EXECUTOR_H
