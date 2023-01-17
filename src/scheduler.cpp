#include "ar/scheduler.hpp"

#include <utility>

using namespace AsyncRuntime;

Scheduler::Scheduler(std::vector<Processor*>  p) :
        is_continue{true}
        , processors(std::move(p))
        , notify_inc(0)
{
    scheduler_th.Submit([this] { SchedulerLoop(); });
}


Scheduler::~Scheduler()
{
    is_continue.store(false, std::memory_order_relaxed);
    delayed_task_cv.notify_one();
    scheduler_th.Join();
}


void Scheduler::SetProcessors(const std::vector<Processor *> &p)
{
    std::lock_guard<std::mutex> lock(processors_mutex);
    processors = p;
}


void Scheduler::SchedulerLoop() {
    while (is_continue.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex> lock(delayed_task_mutex);
        if (!delayed_task.empty()) {
            Task *task = delayed_task.top();

            if (task) {
                int64_t delay = task->GetDelay();
                if (delay <= 0) {
                    ScheduleTask(task);
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


void Scheduler::Post(Task *task)
{
    if (task->GetDelay() <= 0) {
        ScheduleTask(task);
    } else {
        std::unique_lock<std::mutex> lock(delayed_task_mutex);
        delayed_task.push(task);
        delayed_task_cv.notify_one();
    }
}


std::optional<Task *> Scheduler::Steal()
{
    return run_queue.steal();
}


bool Scheduler::IsSteal() const
{
    return !run_queue.empty();
}


void Scheduler::ScheduleTask(Task *task)
{
    {
        std::lock_guard<std::mutex> lock(run_queue_mutex);
        run_queue.push(task);
    }

    std::lock_guard<std::mutex> lock(processors_mutex);
    bool notified = false;
    for (size_t i = 0; i < processors.size(); ++i) {
        notify_inc = (notify_inc + 1) % processors.size();
        auto p_state = processors[notify_inc]->GetState();
        if (p_state != Processor::EXECUTE) {
            notified = true;
            processors[notify_inc]->Notify();
            break;
        }
    }

    if (!notified) {
        processors[notify_inc]->Notify();
    }
}
