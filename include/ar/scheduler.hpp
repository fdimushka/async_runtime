#ifndef AR_WORK_SCHEDULER_H
#define AR_WORK_SCHEDULER_H

#include "ar/task.hpp"
#include "ar/processor.hpp"

#include <random>

namespace AsyncRuntime {

    class Scheduler {
        typedef std::priority_queue<std::shared_ptr<task>, std::vector < std::shared_ptr<task> >, task::less_than_by_delay_ptr>
        TasksPq;
    public:
        explicit Scheduler(std::vector<Processor*>  processors);
        ~Scheduler();

        void SetProcessors(const std::vector<Processor*>&  processors);

        void Post(std::shared_ptr<task> task);

        [[nodiscard]] std::thread::id GetThreadId() const;

        std::shared_ptr<task> Steal();
        [[nodiscard]] bool IsSteal() const;
    private:
        std::random_device rd;
        std::mt19937 gen;
        std::uniform_int_distribution<> distr;
        void ScheduleTask(std::shared_ptr<task> task);
        void SchedulerLoop();

        size_t notify_inc;
        ThreadExecutor scheduler_th;
        std::atomic_bool is_continue;
        oneapi::tbb::concurrent_queue<std::shared_ptr<task>> run_queue;
        TasksPq delayed_task;
        std::condition_variable delayed_task_cv;
        std::mutex delayed_task_mutex;
        std::mutex delayed_lock_mutex;
        std::mutex run_queue_mutex;
        std::mutex processors_mutex;
        std::vector<Processor *> processors;
    };
}
#endif //AR_WORK_SCHEDULER_H
