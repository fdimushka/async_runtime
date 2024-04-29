#include "ar/processor.hpp"
#include "ar/executor.hpp"
#include "ar/profiler.hpp"
#include "ar/processor_group.hpp"
#include "trace.h"


using namespace AsyncRuntime;

Processor::Processor(int pid, const CPU & cpu) :
        BaseObject(),
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
    thread_executor.Join();
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

        std::shared_ptr<task> task = Steal();

        if (!task) {
            task = StealGlobal();
        }

        if (task) {
            ExecuteTask(task, task->get_execution_state());
        } else {
            WaitTask();
        }
    }
}

void Processor::ExecuteTask(const std::shared_ptr<task> & task, const task::execution_state &executor_state) {
    assert(task != nullptr);

    state.store(EXECUTE, std::memory_order_relaxed);
    PROFILER_TASK_WORK_TIME(task->GetOriginId());
    task::execution_state new_state = executor_state;
    new_state.processor = GetID();
    task->execute(new_state);
}

void Processor::Post(const std::shared_ptr<task> & task) {
    std::unique_lock<std::mutex> lock(mutex);
    notify_count++;

    if (task->get_execution_state().work_group != INVALID_OBJECT_ID)
        local_run_queue[task->get_execution_state().work_group].push(task);
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

std::shared_ptr<task> Processor::StealGlobal() {
    std::lock_guard<std::mutex> lock(group_mutex);
    for (auto *group: groups) {
        auto task = group->Steal(GetID());
        if (task) {
            return task;
        }
    }
    return {};
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

std::shared_ptr<task> Processor::Steal()
{
    for (int group_id : rq_by_priority) {
        std::shared_ptr<task> task;
        if (local_run_queue[group_id].try_pop(task))
            return task;
    }

    return {};
}

std::shared_ptr<task> Processor::Pop()
{
    for (int group_id : rq_by_priority) {
        std::shared_ptr<task> task;
        if (local_run_queue[group_id].try_pop(task))
            return task;
    }

    return {};
}

std::shared_ptr<task> Processor::Steal(ObjectID group_id)
{
    std::shared_ptr<task> task;
    if (local_run_queue[group_id].try_pop(task))
        return task;
    return {};
}
