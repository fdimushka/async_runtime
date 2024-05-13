#ifndef AR_TBB_DELAYED_SCHEDULER_H
#define AR_TBB_DELAYED_SCHEDULER_H

#include "ar/task.hpp"
#include "ar/thread_executor.hpp"
#include <oneapi/tbb.h>

namespace AsyncRuntime {

    class TBBDelayedScheduler {
        typedef oneapi::tbb::concurrent_priority_queue<task *, task::less_than_by_delay_ptr> TasksPq;
    public:
        TBBDelayedScheduler();
        ~TBBDelayedScheduler();

        TBBDelayedScheduler(const TBBDelayedScheduler&) = delete;
        TBBDelayedScheduler(TBBDelayedScheduler&&) = delete;
        TBBDelayedScheduler& operator =(const TBBDelayedScheduler&) = delete;
        TBBDelayedScheduler& operator =(TBBDelayedScheduler&&) = delete;


        void Run(const std::function<void(task *)> &task_callback);
        void Terminate();

        void Post(task *task);
    private:
        void ExecuteTask(task *task);
        void Loop();

        std::function<void(task *)> exec_task_callback;
        ThreadExecutor scheduler_th;
        std::atomic_bool is_continue;
        oneapi::tbb::concurrent_queue<task *> rq;
        TasksPq delayed_task;
        std::condition_variable delayed_task_cv;
        std::mutex delayed_task_mutex;
    };
}

#endif //AR_TBB_DELAYED_SCHEDULER_H
