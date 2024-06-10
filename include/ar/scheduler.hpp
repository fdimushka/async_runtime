#ifndef AR_WORK_SCHEDULER_H
#define AR_WORK_SCHEDULER_H

#include "ar/task.hpp"
#include "ar/thread_executor.hpp"

#include <random>

namespace AsyncRuntime {

    class Scheduler {
        typedef std::priority_queue<task *, std::vector < task * >, task::less_than_by_delay_ptr>
        TasksPq;
    public:
        explicit Scheduler(const std::function<void(task *)> &task_callback);
        ~Scheduler();

        void Post(task *task);
    private:
        void SchedulerLoop();

        std::function<void(task *)> task_callback;
        ThreadExecutor scheduler_th;
        std::atomic_bool is_continue;
        TasksPq delayed_task;
        std::condition_variable delayed_task_cv;
        std::mutex delayed_task_mutex;
    };
}
#endif //AR_WORK_SCHEDULER_H
