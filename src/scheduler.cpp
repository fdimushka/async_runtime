#include "ar/scheduler.hpp"
#include "ar/runtime.hpp"

#include <utility>

using namespace AsyncRuntime;

Scheduler::Scheduler(const std::function<void(task *)> &task_callback)
    : is_continue{true}
    , task_callback(task_callback) {
    scheduler_th.Submit([this] { SchedulerLoop(); });
}


Scheduler::~Scheduler() {
    is_continue.store(false, std::memory_order_relaxed);
    delayed_task_cv.notify_one();
    scheduler_th.Join();
}

void Scheduler::SchedulerLoop() {
    while (is_continue.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex> lock(delayed_task_mutex);
        if (!delayed_task.empty()) {
            task *task = delayed_task.top();

            if (task) {
                int64_t delay = task->get_delay();
                if (delay <= 0) {
                    task_callback(task);
                    delayed_task.pop();
                } else {
                    delayed_task_cv.wait_for(lock, std::chrono::microseconds(delay));
                }
            }
        } else {
            delayed_task_cv.wait(lock);
        }
    }
}


void Scheduler::Post(task *task) {
    std::unique_lock<std::mutex> lock(delayed_task_mutex);
    delayed_task.push(task);
    delayed_task_cv.notify_one();
}