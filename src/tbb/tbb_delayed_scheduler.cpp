#include "tbb_delayed_scheduler.h"

using namespace AsyncRuntime;

TBBDelayedScheduler::TBBDelayedScheduler() : is_continue{true} {
}

TBBDelayedScheduler::~TBBDelayedScheduler() {
    Terminate();
}

void TBBDelayedScheduler::Run(const std::function<void(const std::shared_ptr<task> &)> &task_callback) {
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
        std::shared_ptr<task> task;
        if (delayed_task.try_pop(task)) {
            int64_t delay = task->get_delay();
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

void TBBDelayedScheduler::Post(const std::shared_ptr<task> &task) {
    if (task->get_delay() <= 0) {
        ExecuteTask(task);
    } else {
        delayed_task.push(task);
        delayed_task_cv.notify_one();
    }
}

void TBBDelayedScheduler::ExecuteTask(const std::shared_ptr<task> &task) {
    if (exec_task_callback) {
        exec_task_callback(task);
    }
}