#include "tbb_delayed_scheduler.h"

using namespace AsyncRuntime;

TBBDelayedScheduler::TBBDelayedScheduler() : is_continue{true} {
}

TBBDelayedScheduler::~TBBDelayedScheduler() {
    Terminate();
}

void TBBDelayedScheduler::Run(const std::function<void(Task *)> &task_callback) {
    exec_task_callback = task_callback;
    scheduler_th.Submit([this] { Loop(); });
}

void TBBDelayedScheduler::Terminate() {
    is_continue.store(false, std::memory_order_relaxed);
    delayed_task_cv.notify_one();
    scheduler_th.Join();
}

void TBBDelayedScheduler::Loop() {
    while (is_continue.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex> lock(delayed_task_mutex);
        Task *task;
        if (delayed_task.try_pop(task)) {
            int64_t delay = task->GetDelay();
            if (delay <= 0) {
                ExecuteTask(task);
            } else {
                delayed_task.push(task);
                delayed_task_cv.wait_for(lock, std::chrono::microseconds(delay));
            }
        } else {
            delayed_task_cv.wait(lock);
        }
    }
}

void TBBDelayedScheduler::Post(Task *task) {
    if (task->GetDelay() <= 0) {
        ExecuteTask(task);
    } else {
        delayed_task.push(task);
        delayed_task_cv.notify_one();
    }
}

void TBBDelayedScheduler::ExecuteTask(Task *task) {
    if (exec_task_callback) {
        exec_task_callback(task);
    }
}