#include "tbb_stream.h"

using namespace AsyncRuntime;

TBBStream::TBBStream(EntityTag t, uint16_t i)
        : tag(t), index(i), context(oneapi::tbb::task_group_context::isolated), tg(context) {}

TBBStream::TBBStream()
        : tag(0), index(0), context(oneapi::tbb::task_group_context::isolated), tg(context) {}

TBBStream::~TBBStream() {
    Terminate();
    tg.wait();
};

void TBBStream::Terminate() {
    context.cancel_group_execution();
}

void TBBStream::Reset() {
    context.reset();
}

void TBBStream::Post(const std::shared_ptr<task> & task) {
    rq.push(task);
    tg.run([this] { ExecuteTask(); });
}

void TBBStream::SetTag(int64_t t) {
    tag = t;
}

void TBBStream::SetIndex(int64_t i) {
    index = i;
}

void TBBStream::ExecuteTask() {
    std::shared_ptr<task> task;
    if (rq.try_pop(task)) {
        assert(task);
        auto executor_state = task->get_execution_state();
        executor_state.tag = tag;
        task->execute(executor_state);
    }
}