#include "ar/scheduler.hpp"
#include "ar/runtime.hpp"

#include <utility>

using namespace AsyncRuntime;

Scheduler::Scheduler(std::vector<Processor *> p) :
        is_continue{true}, processors(std::move(p)), notify_inc(0), gen(rd()) {
    distr = std::uniform_int_distribution<>(0, processors.size() - 1);
    scheduler_th.Submit([this] { SchedulerLoop(); });
}


Scheduler::~Scheduler() {
    is_continue.store(false, std::memory_order_relaxed);
    delayed_task_cv.notify_one();
    scheduler_th.Join();
}

std::thread::id Scheduler::GetThreadId() const {
    return scheduler_th.GetThreadId();
}

void Scheduler::SetProcessors(const std::vector<Processor *> &p) {
    std::lock_guard<std::mutex> lock(processors_mutex);
    processors = p;
}


void Scheduler::SchedulerLoop() {
    while (is_continue.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex> lock(delayed_task_mutex);
        if (!delayed_task.empty()) {
            std::shared_ptr<task> task = delayed_task.top();

            if (task) {
                int64_t delay = task->get_delay();
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


void Scheduler::Post(std::shared_ptr<task> task) {
    if (task->get_delay() <= 0) {
        ScheduleTask(task);
    } else {
        std::unique_lock<std::mutex> lock(delayed_task_mutex);
        delayed_task.push(task);
        delayed_task_cv.notify_one();
    }
}


std::shared_ptr<task> Scheduler::Steal() {
    std::shared_ptr<task> task;
    if (run_queue.try_pop(task)) {
        return task;
    } else {
        return {};
    }
}


bool Scheduler::IsSteal() const {
    return !run_queue.empty();
}


void Scheduler::ScheduleTask(std::shared_ptr<task> task) {
    auto state = task->get_execution_state();
    ObjectID pid = state.processor;
    if (pid == INVALID_OBJECT_ID || pid <= 0 || pid >= processors.size()) {
        pid = distr(gen);
        state.processor = pid;
        task->set_execution_state(state);
    }

    {
        std::lock_guard<std::mutex> lock(run_queue_mutex);
        run_queue.push(task);
        processors[pid]->Notify();
    }
//
//    std::lock_guard<std::mutex> lock(processors_mutex);
//    bool notified = false;
//    for (size_t i = 0; i < processors.size(); ++i) {
//        notify_inc = (notify_inc + 1) % processors.size();
//        auto p_state = processors[notify_inc]->GetState();
//        if (p_state != Processor::EXECUTE) {
//            notified = true;
//            processors[notify_inc]->Notify();
//            break;
//        }
//    }
//
//    if (!notified) {
//        processors[notify_inc]->Notify();
//    }
}
