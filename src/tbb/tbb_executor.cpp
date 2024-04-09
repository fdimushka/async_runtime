#include "ar/tbb_executor.hpp"
#include "ar/logger.hpp"
#include "ar/runtime.hpp"
#include "numbers.h"
#include <utility>

using namespace AsyncRuntime;

TBBExecutor::TBBExecutor(const std::string &name_,
                         int numa_node,
                         const std::vector<AsyncRuntime::CPU> &cpus,
                         std::vector<WorkGroupOption> work_groups_option)
        : IExecutor(name_, kCPU_EXECUTOR), main_stream(INVALID_OBJECT_ID, 0) {
    std::vector<tbb::numa_node_id> numa_indexes = tbb::info::numa_nodes();
    int numa_node_index = numa_node;
    if (numa_node_index > numa_indexes.size() - 1) { numa_node_index = 0; }
    task_arena.initialize(tbb::task_arena::constraints(numa_indexes[numa_node_index], cpus.size()));

    for (int i = 0; i < MAX_ENTITIES; ++i) {
        streams[i].SetIndex(i);
        free_streams.push(&streams[i]);
    }

    auto schedule_callback = [this](Task *task) {
        Post(task);
    };

    main_delayed_scheduler.Run(schedule_callback);

    for (const auto & wg : work_groups_option) {
        auto scheduler = std::make_unique<TBBDelayedScheduler>();
        scheduler->Run(schedule_callback);
        delayed_schedulers.push_back(std::move(scheduler));
    }
}

TBBExecutor::~TBBExecutor() noexcept {
    main_delayed_scheduler.Terminate();
    for (const auto &scheduler : delayed_schedulers) {
        scheduler->Terminate();
    }
    main_stream.Terminate();
    task_arena.terminate();
}

std::vector<std::thread::id> TBBExecutor::GetThreadIds() {
    return {};
}

uint16_t TBBExecutor::AddEntity(void *ptr) {
    IExecutor::AddEntity(ptr);
    TBBStream *stream = nullptr;
    if (free_streams.try_pop(stream)) {
        stream->Reset();
        return stream->GetIndex();
    } else {
        return 0;
    }
}

void TBBExecutor::DeleteEntity(uint16_t id) {
    IExecutor::DeleteEntity(id);
    if (id >=0 && id < MAX_ENTITIES) {
        streams[id].Terminate();
        free_streams.push(&streams[id]);
    }
}

void TBBExecutor::SetIndex(int i) {
    IExecutor::SetIndex(i);
    for (auto &stream : streams) {
        stream.SetTag(Numbers::Pack(i, stream.GetIndex()));
    }
}

void TBBExecutor::Post(Task *task) {
    task->SetExecutorExecutorState(this);

    task_arena.enqueue([this, task]() {
        const auto &execute_state = task->GetExecutorState();
        if (task->GetDelay() <= 0) {
            PostToStream(task, execute_state.entity_tag);
        } else {
            if (execute_state.work_group != INVALID_OBJECT_ID) {
                delayed_schedulers[execute_state.work_group]->Post(task);
            } else {
                main_delayed_scheduler.Post(task);
            }
        }
    });
}

void TBBExecutor::PostToStream(Task *task, EntityTag tag) {
    if (tag != INVALID_OBJECT_ID) {
        uint16_t executor_index;
        uint16_t entity_index;
        Numbers::Unpack(tag, executor_index, entity_index);
        if (entity_index >= 0 && entity_index < MAX_ENTITIES) {
            streams[entity_index].Post(task);
        } else {
            main_stream.Post(task);
        }
    } else {
        main_stream.Post(task);
    }
}