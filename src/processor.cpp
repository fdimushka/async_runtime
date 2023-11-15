#include "ar/processor.hpp"
#include "ar/executor.hpp"
#include "ar/profiler.hpp"
#include "ar/processor_group.hpp"
#include "trace.h"


using namespace AsyncRuntime;

Processor::Processor(int pid, const CPU & cpu) :
        BaseObject(),
        local_run_queue(MAX_GROUPS_COUNT),
        is_continue{true},
        notify_count{0},
        state{IDLE} {

    id = pid;

    thread_executor.Submit([this] {
        PROFILER_ADD_EVENT(1, Profiler::NEW_THREAD);
        Work();
        PROFILER_ADD_EVENT(1, Profiler::DELETE_THREAD);
    });

    int error = thread_executor.SetAffinity(cpu);
    if (error != 0) {
        AR_LOG_SS(Warning, "Processor thread not affinity to cpu: " << cpu.id)
    }
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
    return thread_executor.GetThreadId();
}


std::vector<ProcessorGroup *> Processor::GetGroups()
{
    std::lock_guard<std::mutex> lock(group_mutex);
    std::vector<ProcessorGroup *> g = groups;
    return std::move(g);
}


size_t Processor::GetGroupsSize()
{
    std::lock_guard<std::mutex> lock(group_mutex);
    size_t size = groups.size();
    return size;
}


void Processor::AddGroup(ProcessorGroup *group)
{
    std::lock_guard<std::mutex> lock(group_mutex);
    std::priority_queue<ProcessorGroup *,
                        std::vector<ProcessorGroup *>,
                        ProcessorGroup::GreaterThanByPriority> pq;
    pq.push(group);
    for (auto *g: groups) {
        pq.push(g);
    }

    rq_by_priority.clear();
    groups.clear();
    while (!pq.empty()) {
        groups.push_back(pq.top());
        rq_by_priority.push_back(pq.top()->GetID());
        pq.pop();
    }
}


bool Processor::IsInGroup(const ProcessorGroup *group)
{
    std::lock_guard<std::mutex> lock(group_mutex);
    return std::any_of(groups.begin(),
                       groups.end(),
                       [group](const auto *g) { return g->GetID() == group->GetID(); });
}


void Processor::Work() {
    //wait run
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (cv.wait_for(lock, std::chrono::milliseconds(1000)) == std::cv_status::timeout) {
            //warning
        }
    }

    while (is_continue.load(std::memory_order_relaxed)) {
        state.store(IDLE, std::memory_order_relaxed);

        std::optional<Task *> task = Steal();

        if (!task.has_value()) {
            task = StealGlobal();
        }

        if (task.has_value()) {
            ExecuteTask(task.value(), task.value()->GetExecutorState());
        } else {
            WaitTask();
        }
    }
}


void Processor::ExecuteTask(Task *task, const ExecutorState &executor_state) {
    assert(task != nullptr);

    state.store(EXECUTE, std::memory_order_relaxed);
    {
        //auto t_start = std::chrono::high_resolution_clock::now();
        PROFILER_TASK_WORK_TIME(task->GetOriginId());
        task->SetProcessorExecutorState(GetID());
        task->Execute(executor_state);
//        auto t_end = std::chrono::high_resolution_clock::now();
//        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
//        if (elapsed_time_ms > 30) {
//            std::cout << std::this_thread::get_id() << " " <<  elapsed_time_ms << std::endl;
//        }
    }
    delete task;
}


void Processor::Post(Task *task) {
    std::unique_lock<std::mutex> lock(mutex);
    notify_count++;

    if (task->GetExecutorState().work_group != INVALID_OBJECT_ID)
        local_run_queue[task->GetExecutorState().work_group].push(task);
    else
        local_run_queue[0].push(task);

    cv.notify_one();
}


void Processor::Notify() {
    std::unique_lock<std::mutex> lock(mutex);
    notify_count++;
    cv.notify_one();
}


void Processor::WaitTask() {
    std::unique_lock<std::mutex> lock(mutex);
    while (notify_count == 0 && is_continue.load(std::memory_order_relaxed)) {
        state.store(WAIT, std::memory_order_relaxed);
        if (!IsStealGlobal())
            cv.wait_for(lock, std::chrono::milliseconds(1000));
        else
            break;
    }
    notify_count--;

    if (notify_count < 0) notify_count = 0;
}


bool Processor::IsStealGlobal()
{
    std::lock_guard<std::mutex> lock(group_mutex);
    for(const auto *group : groups) {
        if (group->IsSteal()) {
            return true;
        }
    }
    return false;
}


std::optional<Task *> Processor::StealGlobal() {
    std::lock_guard<std::mutex> lock(group_mutex);
    for (auto *group: groups) {
        auto task = group->Steal(GetID());
        if (task) {
            return task;
        }
    }
    return std::nullopt;
}


bool Processor::IsSteal() const
{
    for (int group_id : rq_by_priority) {
        if (!local_run_queue[group_id].empty()) {
            return true;
        }
    }
    return false;
}


bool Processor::IsSteal(ObjectID group_id) const
{
    return !local_run_queue[group_id].empty();
}


std::optional<Task *> Processor::Steal()
{
    for (int group_id : rq_by_priority) {
        auto task = local_run_queue[group_id].steal();
        if (task)
            return task;
    }

    return std::nullopt;
}

std::optional<Task *> Processor::Pop()
{
    for (int group_id : rq_by_priority) {
        auto task = local_run_queue[group_id].pop();
        if (task)
            return task;
    }

    return std::nullopt;
}


std::optional<Task *> Processor::Steal(ObjectID group_id)
{
    return local_run_queue[group_id].steal();
}
