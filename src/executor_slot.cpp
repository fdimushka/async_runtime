#include "executor_slot.h"
#include "numbers.h"

#define MAX_STEALS 24

using namespace AsyncRuntime;

ExecutorSlot::ExecutorSlot(ObjectID _id,
                           const std::string &name,
                           const std::vector<AsyncRuntime::CPU> &cpus)
        : id(_id)
        , name(name)
        , workers{cpus.size()}
        , threads{cpus.size()}
        , notifier{cpus.size()}
{
    spawn(cpus);
}

ExecutorSlot::~ExecutorSlot() {
    done = true;

    notifier.notify(true);

    for(auto& t : threads){
        t.join();
    }
}

inline size_t ExecutorSlot::num_workers() const noexcept {
    return workers.size();
}

void ExecutorSlot::spawn(const std::vector<AsyncRuntime::CPU> &cpus) {
    std::mutex mutex;
    std::condition_variable cond;
    size_t n=0;
    for (size_t id=0; id<cpus.size(); ++id) {
        workers[id].id = id;
        workers[id].cpu_id = cpus[id].id;
        workers[id].vtm = id;
        workers[id].executor = this;
        workers[id].waiter = &notifier._waiters[id];

        threads[id] = std::thread([&, &w=workers[id]] () {
            w.thread = &threads[w.id];
            {
                std::scoped_lock lock(mutex);
                wids[std::this_thread::get_id()] = w.id;
                if(n++; n == num_workers()) {
                    cond.notify_one();
                }
            }

            task* t = nullptr;

            while(!done) {
                exploit_task(w, t);

                if(!t) {
                    notifier.prepare_wait(w.waiter);
                    if (explore_task(w, t)) {
                        notifier.cancel_wait(w.waiter);
                        notifier.notify(false);
                        continue;
                    }
                }

                if(t) {
                    w.execute.store(true, std::memory_order_relaxed);
                    invoke(w, t);
                    delete t;
                    t = nullptr;
                } else {
                    w.execute.store(false, std::memory_order_relaxed);
                    notifier.commit_wait(w.waiter);
                }
            }
        });

        AsyncRuntime::SetAffinity(threads[id], cpus[id]);
    }

    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock, [&](){ return n==cpus.size(); });
}

void ExecutorSlot::exploit_task(Worker& w, task*& t) {
    {
        std::lock_guard<std::mutex> lock(w.wsq_mutex);
        t = w.wsq.pop();
    }

    if (t) {
        return;
    }

    t = wsq.steal();
    if (t) {
        return;
    }

    for (auto &other_w: workers) {
        if (w.id != other_w.id && other_w.execute.load()) {
            t = other_w.wsq.steal();
            if (t) {
                return;
            }
        }
    }
}

bool ExecutorSlot::explore_task(Worker& w, task*& t) {
    for(int i = 0; i < MAX_STEALS; ++i) {

        if (!w.wsq.empty()) {
            return true;
        }

        if (!wsq.empty()) {
            return true;
        }

        for (auto &other_w: workers) {
            if (w.id != other_w.id && other_w.execute.load()) {
                if (!other_w.wsq.empty()) {
                    return true;
                }
            }
        }
    }

    std::this_thread::yield();
    return false;
}

void ExecutorSlot::invoke(Worker& w, task* t) {
    task::execution_state new_state = t->get_execution_state();
    new_state.processor = w.cpu_id;

    t->execute(new_state);
//    if (m_executed_tasks_count) {
//        m_executed_tasks_count->Increment();
//    }
}

void ExecutorSlot::add_entity() {
    if (m_entities_count) {
        m_entities_count->Increment();
    }
    entities_count.fetch_add(1, std::memory_order_relaxed);
}

void ExecutorSlot::delete_entity() {
    if (m_entities_count) {
        m_entities_count->Decrement();
    }
    if (entities_count.fetch_sub(1, std::memory_order_relaxed) <= 0) {
        entities_count.store(0, std::memory_order_relaxed);
    }
}

int ExecutorSlot::get_util() {
    return entities_count.load(std::memory_order_relaxed);
}

void ExecutorSlot::post(task *task) {
//    if (m_posted_tasks_count) {
//        m_posted_tasks_count->Increment();
//    }
    auto &state = task->get_execution_state();
    if (state.processor != INVALID_OBJECT_ID) {
        for (auto &w : workers) {
            if (w.cpu_id == state.processor) {
                std::lock_guard<std::mutex> lock(w.wsq_mutex);
                w.wsq.push(task, 0);
                notifier.notify_waiter(w.waiter);
                return;
            }
        }

        {
            std::lock_guard<std::mutex> lock(wsq_mutex);
            wsq.push(task, 0);
            notifier.notify(false);
        }
    } else {
        std::lock_guard<std::mutex> lock(wsq_mutex);
        wsq.push(task, 0);
        notifier.notify(false);
    }
}

