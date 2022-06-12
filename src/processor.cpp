#include "ar/processor.hpp"
#include "ar/executor.hpp"
#include "ar/profiler.hpp"

#include "trace.h"


using namespace AsyncRuntime;


Processor::Processor(Executor* executor_) :
    BaseObject(),
    executor(executor_),
    is_continue{true},
    notify_count{0},
    executor_state{executor_, this},
    state{IDLE}
{
    assert(executor != nullptr);
    thread_executor.Submit([this] { Work(); });
}


Processor::~Processor()
{
    is_continue.store(false, std::memory_order_relaxed);
    thread_executor.Join();
}


void Processor::Run()
{
    is_continue.store(true, std::memory_order_relaxed);
    cv.notify_one();
}


void Processor::Terminate()
{
    is_continue.store(false, std::memory_order_relaxed);
    cv.notify_one();
}


Processor::State Processor::GetState()
{
    return state.load(std::memory_order_relaxed);
}


std::thread::id Processor::GetThreadId() const
{
    if(!thread_executor.GetThreads().empty()) {
        return thread_executor.GetThreadIds().front();
    } else {
        return std::this_thread::get_id();
    }
}


void Processor::Work()
{
    //wait run
    {
        std::unique_lock<std::mutex> lock(mutex);
        if(cv.wait_for(lock, std::chrono::milliseconds(1000)) == std::cv_status::timeout) {
            //warning
        }
    }

    while (is_continue.load(std::memory_order_relaxed))
    {
        state.store(IDLE, std::memory_order_relaxed);
        auto task = ConsumeWork();
        if (task) {
            ExecuteTask(task.value());
        }else{
            WaitTask();
        }
    }
}


std::optional<Task *> Processor::ConsumeWork()
{
    std::optional<Task*> task;
    task = StealWorkLocal();

    if(executor != nullptr) {
        if (!task) {
            task = StealWorkGlobal();
        }

        if (!task) {
            task = StealWorkOnOthers();
        }
    }

    return task;
}


void Processor::ExecuteTask(Task *task)
{
    assert(task != nullptr);

    state.store(EXECUTE, std::memory_order_relaxed);
    //PROFILE_BEGIN_TASK(task);
    task->Execute(executor_state);
    //PROFILE_END_TASK(task);
    delete task;
}


void Processor::Post(Task *task)
{
    std::unique_lock<std::mutex> lock(mutex);
    notify_count++;
    local_run_queue.push(task);
    cv.notify_one();
}


void Processor::Notify()
{
    std::unique_lock<std::mutex> lock(mutex);
    notify_count++;
    cv.notify_one();
}


void Processor::WaitTask()
{
    std::unique_lock<std::mutex>  lock(mutex);
    while(notify_count == 0 && is_continue.load(std::memory_order_relaxed)) {
        state.store(WAIT, std::memory_order_relaxed);
        if(executor->run_queue.empty())
            cv.wait_for(lock, std::chrono::milliseconds(1000));
        else
            break;
    }
    notify_count--;

    if(notify_count < 0) notify_count = 0;
}


std::optional<Task*> Processor::StealWorkGlobal()
{
    return executor->run_queue.steal();
}


std::optional<Task*> Processor::StealWorkLocal()
{
    return local_run_queue.steal();
}


std::optional<Task*> Processor::StealWorkOnOthers()
{
    std::optional<Task*> task = std::nullopt;
    for (int i = 0; i < executor->processors.size() && !task; i++) {
        auto processor = executor->processors[i];
        if (processor->GetID() != GetID() &&
            processor->GetState() == EXECUTE &&
            !processor->local_run_queue.empty())
        {
            task = processor->StealWorkLocal();
        }
    }

    return task;
}