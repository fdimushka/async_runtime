#include "ar/processor.hpp"
#include "ar/executor.hpp"
#include "ar/profiler.hpp"
#include "ar/processor_group.hpp"
#include "trace.h"


using namespace AsyncRuntime;

Processor::Processor(int pid, const CPU &cpu) :
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

Processor::~Processor() {
    is_continue.store(false, std::memory_order_relaxed);
}

void Processor::Run() {
    is_continue.store(true, std::memory_order_relaxed);
    cv.notify_one();
}

void Processor::Terminate() {
    is_continue.store(false, std::memory_order_relaxed);
    cv.notify_one();
    thread_executor.Join();
}

Processor::State Processor::GetState() {
    return state.load(std::memory_order_relaxed);
}

std::thread::id Processor::GetThreadId() const {
    return thread_executor.GetThreadId();
}

std::vector<ProcessorGroup *> Processor::GetGroups() {
    return groups;
}

size_t Processor::GetGroupsSize() {
    return groups.size();
}

void Processor::AddGroup(ProcessorGroup *group) {
    groups.push_back(group);
}

bool Processor::IsInGroup(const ProcessorGroup *group) {
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

        task *task = Pop();

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

void Processor::ExecuteTask(task *task, const task::execution_state &executor_state) {
    state.store(EXECUTE, std::memory_order_relaxed);
    notify_count.store(0, std::memory_order_relaxed);
    task::execution_state new_state = executor_state;
    new_state.processor = GetID();
    task->execute(new_state);
    delete task;
}

void Processor::Post(task *task) {
    notify_count++;
    {
        const auto &state = task->get_execution_state();
        ObjectID wg = state.work_group != INVALID_OBJECT_ID? state.work_group : 0;
        std::lock_guard<std::mutex> lock(task_queue_mutex);
        task_queue[wg].push(task, static_cast<unsigned int>(TaskPriority::NORMAL));
    }
    cv.notify_one();
}

void Processor::Notify() {
    notify_count++;
    cv.notify_one();
}

void Processor::WaitTask() {
    std::unique_lock<std::mutex> lock(mutex);
    while (notify_count.load(std::memory_order_relaxed) == 0 && is_continue.load(std::memory_order_relaxed)) {
        state.store(WAIT, std::memory_order_relaxed);
        std::this_thread::yield();
        cv.wait(lock);
//        if (!IsStealGlobal())
//            cv.wait_for(lock, std::chrono::milliseconds(1000));
//        else
//            break;
    }
    notify_count--;

    if (notify_count < 0) notify_count = 0;
}

task *Processor::StealGlobal() {
    std::lock_guard<std::mutex> lock(group_mutex);
    for (auto *group: groups) {
        auto task = group->Steal(GetID());
        if (task) {
            return task;
        }
    }
    return nullptr;
}

task *Processor::Steal() {
    task *task = nullptr;
    for (auto & group : groups) {
        task = task_queue[group->GetID()].steal();
        if (task != nullptr) {
            return task;
        }
    }

    return nullptr;
}

task *Processor::Pop() {
    std::lock_guard<std::mutex> lock(task_queue_mutex);
    task *task;
    for (auto & group : groups) {
        task = task_queue[group->GetID()].pop();
        if (task != nullptr) {
            return task;
        }
    }
    return nullptr;
}

task *Processor::Steal(ObjectID group_id) {
    if (state.load(std::memory_order_relaxed) == EXECUTE) {
        return task_queue[group_id].steal();
    } else {
        return nullptr;
    }
}
