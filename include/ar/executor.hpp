#ifndef AR_EXECUTOR_H
#define AR_EXECUTOR_H

#include "ar/processor.hpp"

namespace AsyncRuntime {
    class IExecutor: public BaseObject {
    public:
        virtual void Post(Task* task) = 0;
    };


    class Executor  : public IExecutor {
        friend Processor;
    public:
        explicit Executor(const std::string & name_,
                           int max_processors_count_ = std::thread::hardware_concurrency());
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
    private:
        void Spawn();
        void RunQueuePush(Task *task);


        int                         max_processors_count;
        WorkStealQueue<Task*>       run_queue;
        std::mutex                  run_queue_mutex;
        std::vector<Processor*>     processors;
        std::string                 name;
        size_t                      notify_inc;
    };
}

#endif //AR_EXECUTOR_H
