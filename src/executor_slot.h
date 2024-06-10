#ifndef TEYE_STREAMSERVER_EXECUTOR_SLOT_H
#define TEYE_STREAMSERVER_EXECUTOR_SLOT_H

#include <iostream>
#include <map>
#include <atomic>
#include "ar/object.hpp"
#include "ar/task.hpp"
#include "ar/work_steal_queue.hpp"
#include "ar/thread_executor.hpp"
#include "ar/scheduler.hpp"
#include "ar/metricer.hpp"
#include "work_notifier.h"
#include "ar/task_queue.hpp"

namespace AsyncRuntime {
    class ExecutorSlot;
    class ExecutorWorkGroup;

    class Worker {
        friend class ExecutorSlot;
    public:
        inline size_t get_id() const { return id; }

        inline size_t get_cpu_id() const { return cpu_id; }

        inline std::thread* get_thread() const { return thread; }

        inline size_t get_queue_size() const { return wsq.size(); }

        inline size_t get_queue_capacity() const { return static_cast<size_t>(wsq.capacity()); }
    private:
        size_t id;
        size_t cpu_id;
        size_t vtm;
        std::atomic_bool execute = {false};
        ExecutorSlot* executor;
        std::thread* thread;
        WorkNotifier::Waiter* waiter;
        std::default_random_engine rdgen { std::random_device{}() };
        TaskQueue<task*, 1> wsq;
        std::mutex       wsq_mutex;
    };

    class ExecutorSlot {
        friend class ExecutorWorkGroup;
    public:
        ExecutorSlot(ObjectID id, const std::string &name, const std::vector<AsyncRuntime::CPU> &cpus);
        ~ExecutorSlot();

        void post(task *task);

        [[nodiscard]] ObjectID get_id() const { return id; }

        void add_entity();

        void delete_entity();

        int get_util();
    private:
        inline size_t num_workers() const noexcept;

        void spawn(const std::vector<AsyncRuntime::CPU> &cpus);
        void exploit_task(Worker& w, task*& t);
        bool explore_task(Worker& w, task*& t);

        void invoke(Worker& w, task* t);

        ObjectID                        id;
        std::string                     name;
        std::vector<Worker>             workers;
        std::vector<std::thread>        threads;
        TaskQueue<task*, 1>                wsq;
        WorkNotifier                    notifier;
        std::mutex                      wsq_mutex;
        std::unordered_map<std::thread::id, size_t> wids;
        std::atomic<bool> done = {false};
        std::atomic_int    entities_count = {0};
        std::shared_ptr<Mon::Counter>   m_entities_count;
        std::shared_ptr<Mon::Counter>   m_posted_tasks_count;
        std::shared_ptr<Mon::Counter>   m_executed_tasks_count;
    };
}

#endif //TEYE_STREAMSERVER_EXECUTOR_SLOT_H
