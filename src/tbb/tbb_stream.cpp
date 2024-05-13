#include "tbb_stream.h"
#include "ar/executor.hpp"

using namespace AsyncRuntime;

void TBBStream::work_group::post(task *task) {
    //notify_count++;
    //cv.notify_one();
    rq.push(task);
    run();
}

void TBBStream::work_group::wait() {
    tg.wait();
}

void TBBStream::work_group::terminate() {
    is_continue.store(false, std::memory_order_relaxed);
    context.cancel_group_execution();
}

void TBBStream::work_group::reset() {
    context.reset();
}

void TBBStream::work_group::run() {
    tg.run([this] {
        //while (is_continue.load(std::memory_order_relaxed)) {
        task *task = nullptr;
        if (rq.try_pop(task)) {
            assert(task);
            auto executor_state = task->get_execution_state();
            task->execute(executor_state);
            delete task;
        }
        //}
    });
}

void TBBStream::work_group::wait_task() {
    std::unique_lock<std::mutex> lock(mutex);
    while (notify_count == 0 && is_continue.load(std::memory_order_relaxed)) {
        if (rq.empty()) {
            cv.wait_for(lock, std::chrono::milliseconds(1000));
        } else {
            break;
        }
    }
    notify_count--;

    if (notify_count < 0) notify_count = 0;
}


TBBStream::TBBStream(EntityTag t, uint16_t i, const std::vector<WorkGroupOption> &work_groups_option)
        : tag(t), index(i) {}

TBBStream::TBBStream()
        : tag(0), index(0) {}

TBBStream::~TBBStream() {
    Terminate();
};

void TBBStream::Terminate() {
    for(int i = 0; i < MAX_WG_COUNT; ++i) {
        groups[i].terminate();
    }
}

void TBBStream::Reset() {
    for(int i = 0; i < MAX_WG_COUNT; ++i) {
        groups[i].reset();
    }
}

void TBBStream::Post(task *task, int64_t wg) {
    if (wg != INVALID_OBJECT_ID &&
        wg >= 0 &&
        wg < MAX_WG_COUNT) {
        groups[wg].post(task);
    } else {
        groups[0].post(task);
    }
}

void TBBStream::Run(int64_t wg) {
    if (wg != INVALID_OBJECT_ID &&
        wg >= 0 &&
        wg < MAX_WG_COUNT) {
        groups[wg].run();
    }
}

void TBBStream::SetTag(int64_t t) {
    tag = t;
}

void TBBStream::SetIndex(int64_t i) {
    index = i;
}