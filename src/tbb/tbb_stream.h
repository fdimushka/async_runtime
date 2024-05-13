#ifndef AR_TBB_STREAM_H
#define AR_TBB_STREAM_H

#include "ar/task.hpp"
#include "ar/thread_executor.hpp"
#include <oneapi/tbb.h>

namespace AsyncRuntime {
#define MAX_WG_COUNT                10

    class WorkGroupOption;

    class TBBStream {
        class work_group {
        public:
            work_group() : context(oneapi::tbb::task_group_context::isolated), tg(context) {}

            void wait();

            void run();

            void post(task *task);

            void terminate();

            void reset();
        private:
            void wait_task();
            std::mutex mutex;
            std::atomic_bool is_continue = {true};
            std::condition_variable cv;
            std::atomic_int notify_count = {0};
            oneapi::tbb::concurrent_queue<task *> rq;
            oneapi::tbb::task_group_context context;
            oneapi::tbb::task_group tg;
        };

    public:
        explicit TBBStream(EntityTag tag, uint16_t index, const std::vector<WorkGroupOption> &work_groups_option);

        TBBStream();

        ~TBBStream();

        void Terminate();

        void Reset();

        void Run(int64_t wg);

        void Post(task *task, int64_t wg);

        void SetTag(int64_t t);

        void SetIndex(int64_t i);

        EntityTag GetTag() const { return tag; }

        uint16_t GetIndex() const { return index; }

    private:
        EntityTag tag;
        uint16_t index;
        tbb::task_arena arena;
        work_group groups[MAX_WG_COUNT];
    };
}

#endif //AR_TBB_STREAM_H
