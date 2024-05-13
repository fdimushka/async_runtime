#include "tbb_executor.h"
#include "ar/logger.hpp"
#include "ar/runtime.hpp"
#include "numbers.h"
#include <utility>

using namespace AsyncRuntime;

TBBExecutor::TBBExecutor(const std::string &name_,
                         int numa_node,
                         int max_concurrency,
                         std::vector<WorkGroupOption> work_groups_option)
        : IExecutor(name_, kCPU_EXECUTOR), main_stream(INVALID_OBJECT_ID, 0, work_groups_option) {

    for (int i = 0; i < MAX_ENTITIES; ++i) {
        streams[i].SetIndex(i);
        free_streams.push(&streams[i]);
    }

    std::vector<tbb::numa_node_id> numa_indexes = tbb::info::numa_nodes();
    int numa_node_index = numa_node;
    if (numa_node_index >= numa_indexes.size()) { numa_node_index = 0; }

    for (int i = 0; i < work_groups_option.size(); ++i) {
        int concurrency = (int) ((double) max_concurrency * (1.0 / (work_groups_option[i].cap / work_groups_option[i].util)));
        concurrency = std::min(std::max(4, concurrency), max_concurrency);
        tbb::task_arena::priority priority = tbb::task_arena::priority::normal;

        priority = tbb::task_arena::priority::normal;

        task_arenas[i].initialize(tbb::task_arena::constraints(numa_indexes[numa_node_index], concurrency), 1, priority);
//        task_arenas[i].execute([this, &i]() {
//            main_stream.Run(i);
////            for (int s = 0; s < MAX_ENTITIES; ++s) {
////                streams[s].Run(i);
////            }
//        });
    }

    auto schedule_callback = [this](task *task) {
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
    for (const auto &scheduler: delayed_schedulers) {
        scheduler->Terminate();
    }
    main_stream.Terminate();
    for (int i = 0; i < MAX_ARENA_COUNT; ++i) {
        task_arenas[i].terminate();
    }
}

std::vector<std::thread::id> TBBExecutor::GetThreadIds() {
    return {};
}

uint16_t TBBExecutor::AddEntity(void *ptr) {
    IExecutor::AddEntity(ptr);
    TBBStream *stream = nullptr;
    if (free_streams.try_pop(stream)) {
        //stream->Reset();
        return stream->GetIndex();
    } else {
        return 0;
    }
}

void TBBExecutor::DeleteEntity(uint16_t id) {
    IExecutor::DeleteEntity(id);
    if (id >=0 && id < MAX_ENTITIES) {
        //streams[id].Terminate();
        free_streams.push(&streams[id]);
    }
}

void TBBExecutor::SetIndex(int i) {
    IExecutor::SetIndex(i);
    for (auto &stream : streams) {
        stream.SetTag(Numbers::Pack(i, stream.GetIndex()));
    }
}

void TBBExecutor::Post(task *task) {
    auto execute_state = task->get_execution_state();
    execute_state.executor = this;
    task->set_execution_state(execute_state);
    tbb::task_arena* arena;
    if (execute_state.work_group != INVALID_OBJECT_ID &&
        execute_state.work_group >= 0 &&
        execute_state.work_group <= MAX_ARENA_COUNT) {
        arena = &task_arenas[execute_state.work_group];
    } else {
        arena = &task_arenas[0];
    }

    arena->enqueue([this, task]() {
        const auto &execute_state = task->get_execution_state();
        if (task->get_delay() <= 0) {
            Enqueue(task, execute_state.tag);
            //PostToStream(task, execute_state.tag, execute_state.work_group);
        } else {
            if (execute_state.work_group != INVALID_OBJECT_ID) {
                delayed_schedulers[execute_state.work_group]->Post(task);
            } else {
                main_delayed_scheduler.Post(task);
            }
        }
    });

}

void TBBExecutor::PostToStream(task *task, EntityTag tag, int64_t wg) {
    if (tag != INVALID_OBJECT_ID) {
        uint16_t executor_index;
        uint16_t entity_index;
        Numbers::Unpack(tag, executor_index, entity_index);
        if (entity_index >= 0 && entity_index < MAX_ENTITIES) {
            streams[entity_index].Post(task, wg);
        } else {
            main_stream.Post(task, wg);
        }
    } else {
        main_stream.Post(task, wg);
    }
}

void TBBExecutor::Enqueue(task *task, EntityTag tag) {
    assert(task);
    auto executor_state = task->get_execution_state();
    executor_state.tag = tag;
    task->execute(executor_state);
    delete task;
}