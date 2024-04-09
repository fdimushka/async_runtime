#ifndef AR_TBB_STREAM_H
#define AR_TBB_STREAM_H

#include "ar/task.hpp"
#include "ar/thread_executor.hpp"
#include <oneapi/tbb.h>

namespace AsyncRuntime {

    class TBBStream {
    public:
        explicit TBBStream(EntityTag tag, uint16_t index);

        TBBStream();

        ~TBBStream();

        void Terminate();

        void Reset();

        void Post(AsyncRuntime::Task *task);

        void SetTag(int64_t t);

        void SetIndex(int64_t i);

        EntityTag GetTag() const { return tag; }
        
        uint16_t GetIndex() const { return index; }
    private:
        void ExecuteTask();

        EntityTag tag;
        uint16_t index;
        oneapi::tbb::concurrent_queue<AsyncRuntime::Task *> rq;
        oneapi::tbb::task_group_context context;
        oneapi::tbb::task_group tg;
    };
}

#endif //AR_TBB_STREAM_H
