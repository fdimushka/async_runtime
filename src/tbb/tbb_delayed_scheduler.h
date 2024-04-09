#ifndef AR_TBB_DELAYED_SCHEDULER_H
#define AR_TBB_DELAYED_SCHEDULER_H

#include "ar/task.hpp"
#include "ar/thread_executor.hpp"
#include <oneapi/tbb.h>

namespace AsyncRuntime {

    class TBBDelayedScheduler {
        //typedef std::priority_queue<Task *, std::vector < Task * >, Task::LessThanByDelay> TasksPq;
        typedef oneapi::tbb::concurrent_priority_queue<Task *, Task::LessThanByDelay> TasksPq;
    public:
        TBBDelayedScheduler();
        ~TBBDelayedScheduler();

        TBBDelayedScheduler(const TBBDelayedScheduler&) = delete;
        TBBDelayedScheduler(TBBDelayedScheduler&&) = delete;
        TBBDelayedScheduler& operator =(const TBBDelayedScheduler&) = delete;
        TBBDelayedScheduler& operator =(TBBDelayedScheduler&&) = delete;


        void Run(const std::function<void(Task *)> &task_callback);
        void Terminate();

        void Post(Task *task);
    private:
        void ExecuteTask(Task *task);
        void Loop();

        std::function<void(Task *)> exec_task_callback;
        ThreadExecutor scheduler_th;
        std::atomic_bool is_continue;
        oneapi::tbb::concurrent_queue<Task*> rq;
        TasksPq delayed_task;
        std::condition_variable delayed_task_cv;
        std::mutex delayed_task_mutex;
    };
}

#endif //AR_TBB_DELAYED_SCHEDULER_H
