#include "ar/executor.hpp"

#include <utility>


using namespace AsyncRuntime;


Executor::Executor(std::string  name_, uint max_processors_count_) :
            name(std::move(name_)),
            notify_inc(0),
            max_processors_count(max_processors_count_),
            is_continue{true}
{
    Spawn();
}


Executor::~Executor()
{
    is_continue.store(false, std::memory_order_relaxed);
    delayed_task_cv.notify_one();
    scheduler_executor.Join();

    for(auto & processor : processors)
    {
        processor->Terminate();
        delete processor;
        processor = nullptr;
    }
    processors.clear();
}


void Executor::Spawn()
{
    scheduler_executor.Submit([this] { SchedulerLoop(); });

    for(int i = 0; i < max_processors_count; ++i) {
        auto *processor = new Processor(this);
        processors.emplace_back(processor);
    }


    for(auto *processor : processors){
        processor->Run();
    }
}


void Executor::SchedulerLoop()
{
    while (is_continue.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex>  lock(delayed_task_mutex);
        if(!delayed_task.empty()) {
            Task *task = delayed_task.top();

            if(task) {
                int64_t delay = task->GetDelay();
                if (delay <= 0) {
                    Post(task);
                    delayed_task.pop();
                }else{
                    delayed_task_cv.wait_for(lock, std::chrono::microseconds (delay));
                }
            }
        }else{
            delayed_task_cv.wait(lock);
        }
    }
}


void Executor::Post(Task *task)
{
    if(task->GetDelay() <= 0) {
        const auto &execute_state = task->GetDesirableExecutor();
        if (execute_state.processor != nullptr) {
            Processor *processor = execute_state.processor;
            processor->Post(task);
        } else {
            RunQueuePush(task);

            bool notified = false;
            for (size_t i = 0; i < processors.size(); ++i) {
                notify_inc = (notify_inc + 1) % processors.size();
                size_t pid = notify_inc;

                auto p_state = processors[pid]->GetState();
                if (p_state != Processor::EXECUTE) {
                    notified = true;
                    processors[pid]->Notify();
                    break;
                }
            }

            if (!notified) {
                processors[notify_inc]->Notify();
            }
        }
    }else{
        std::unique_lock<std::mutex> lock(delayed_task_mutex);
        delayed_task.push(task);
        delayed_task_cv.notify_one();
    }
}


void Executor::RunQueuePush(Task *task)
{
    std::lock_guard<std::mutex> lock(run_queue_mutex);
    run_queue.push(task);
}