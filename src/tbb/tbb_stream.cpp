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

void TBBStream::Post(AsyncRuntime::Task *task) {
    std::cout << "push work to " << index << " " << tag << std::endl;
    rq.push(task);
    tg.run(std::bind(&TBBStream::ExecuteTask, this));
}

void TBBStream::SetTag(int64_t t) {
    tag = t;
}

void TBBStream::SetIndex(int64_t i) {
    index = i;
}

void TBBStream::ExecuteTask() {
    Task *task = nullptr;
    if (rq.try_pop(task)) {
        assert(task);
        ExecutorState executor_state;
        executor_state.entity_tag = tag;
        executor_state.executor = task->GetExecutorState().executor;
        executor_state.work_group = task->GetExecutorState().work_group;
        //executor_state.processor = std::this_thread::get_id();
        task->Execute(executor_state);
        delete task;
    }
}