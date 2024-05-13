#ifndef AR_TBB_EXECUTOR_H
#define AR_TBB_EXECUTOR_H

#include "ar/executor.hpp"
#include "ar/metricer.hpp"
#include "ar/cpu_helper.hpp"
#include <oneapi/tbb.h>
#include "tbb_delayed_scheduler.h"
#include "tbb_stream.h"

namespace AsyncRuntime {
#define MAX_ARENA_COUNT                10
    class TBBExecutor : public IExecutor {
    public:
        TBBExecutor(const std::string &name_,
                    int numa_node,
                    int max_concurrency,
                    std::vector<WorkGroupOption> work_groups_option = {});

        ~TBBExecutor() noexcept override;


        TBBExecutor(const TBBExecutor &) = delete;

        TBBExecutor(TBBExecutor &&) = delete;

        TBBExecutor &operator=(const TBBExecutor &) = delete;

        TBBExecutor &operator=(TBBExecutor &&) = delete;

        uint16_t AddEntity(void *ptr) final;

        void DeleteEntity(uint16_t id) final;

        void SetIndex(int index) final;

        void Post(task *task) final;

        std::vector<std::thread::id> GetThreadIds();
    private:
        void Enqueue(task * task, EntityTag tag);
        void PostToStream(task * task, EntityTag tag, int64_t wg);
        
        TBBDelayedScheduler main_delayed_scheduler;
        std::vector<std::unique_ptr<TBBDelayedScheduler>> delayed_schedulers;
        oneapi::tbb::concurrent_queue<TBBStream*> free_streams;
        TBBStream streams[MAX_ENTITIES];
        TBBStream main_stream;
        tbb::task_arena task_arenas[MAX_ARENA_COUNT];
    };
}

#endif //AR_TBB_EXECUTOR_H
