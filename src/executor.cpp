#include "ar/executor.hpp"
#include "ar/logger.hpp"
#include "ar/runtime.hpp"
#include <utility>
#include "executor_slot.h"
#include "numbers.h"

using namespace AsyncRuntime;

IExecutor::IExecutor(const std::string &ex_name, ExecutorType executor_type)
    : name(ex_name)
    , type(executor_type) {
}

uint16_t IExecutor::AddEntity(void *ptr) {
    entities_count.fetch_add(1, std::memory_order_relaxed);
    return 0;
}


void IExecutor::DeleteEntity(uint16_t id) {
    if (entities_count.fetch_sub(1, std::memory_order_relaxed) <= 0) {
        entities_count.store(0, std::memory_order_relaxed);
    }
}

ExecutorWorkGroup::ExecutorWorkGroup(int id,
                                     const WorkGroupOption & option,
                                     const std::vector<AsyncRuntime::CPU> &cpus,
                                     std::map<size_t, size_t>& cpus_wg) : id(id) {
    int slot_concurrency = (option.slot_concurrency > 0)? option.slot_concurrency : std::thread::hardware_concurrency();
    int max_cpus = (int) ((double) cpus.size() * (1.0 / (option.cap / option.util)));
    max_cpus = std::min(std::max(1, max_cpus), (int)cpus.size());
    int slots_count = max_cpus/slot_concurrency;
    slots_count = std::max(1, slots_count);
    name = option.name;

    for (int i = 0; i < slots_count; ++i) {
        std::vector<AsyncRuntime::CPU> slot_cpus;

        for (int c = 0; c < slot_concurrency; ++c) {
            auto it = min_element(cpus_wg.begin(), cpus_wg.end(),
                                  [](std::pair<size_t, size_t> l, std::pair<size_t, size_t> r) -> bool { return l.second < r.second; });
            it->second++;
            slot_cpus.push_back(cpus[it->first]);
            cpus_peer_slot[cpus[it->first].id] = i;
        }

        auto slot = new ExecutorSlot(i, option.name, slot_cpus);
        slots.push_back(slot);
    }

    scheduler = std::make_unique<Scheduler>([this](task *task) {
        Post(task);
    });
}

ExecutorWorkGroup::~ExecutorWorkGroup() {
    for (auto slot: slots) {
        delete slot;
    }
}

void ExecutorWorkGroup::MakeMetrics(const std::string &executor_name, const std::shared_ptr<Mon::IMetricer> &m) {
    metricer = m;
    if (metricer) {
        workers_count = metricer->MakeCounter("ar_workers_count", {
                {"executor", executor_name},
                {"group",    name},
        });

        slots_count = metricer->MakeCounter("ar_slots_count", {
                {"executor", executor_name},
                {"group",    name},
        });

        for (auto *slot: slots) {
            slot->m_entities_count = metricer->MakeCounter("ar_entities_count", {
                    {"executor", executor_name},
                    {"group",    name},
                    {"slot",     std::to_string(slot->id)},
            });
        }

        workers_count->Increment(cpus_peer_slot.size());
        slots_count->Increment(slots.size());
    }
}

void ExecutorWorkGroup::DeleteEntity(uint16_t id) {
    std::lock_guard<std::mutex> lock(mutex);
    auto slot_it = entities_peer_slot.find(id);
    if (slot_it != entities_peer_slot.end()) {
        entities_peer_slot.erase(slot_it);
    }
}

void ExecutorWorkGroup::Post(task *task) {
    task->set_execution_state_wg(id);
    if (task->get_delay() <= 0) {
        const auto& state = task->get_execution_state();
        if (state.processor != INVALID_OBJECT_ID) {
            auto slot_it = cpus_peer_slot.find(state.processor);
            if (slot_it != cpus_peer_slot.end()) {
                slots[slot_it->second]->post(task);
                return;
            }
        }

        {
            if (state.tag != INVALID_OBJECT_ID) {
                std::lock_guard<std::mutex> lock(mutex);
                uint16_t id, entity;
                Numbers::Unpack(state.tag, id, entity);
                auto slot_it = entities_peer_slot.find(entity);
                if (slot_it == entities_peer_slot.end()) {
                    auto free_slot_it = min_element(slots.begin(), slots.end(),
                                                    [](ExecutorSlot *l, ExecutorSlot *r) -> bool {
                                                        return l->get_util() < r->get_util();
                                                    });
                    auto *free_slot = *free_slot_it;
                    free_slot->add_entity();
                    entities_peer_slot[entity] = free_slot->get_id();
                    free_slot->post(task);
                } else {
                    slots[slot_it->second]->post(task);
                }
            } else {
                slots[0]->post(task);
            }
        }
    } else {
        scheduler->Post(task);
    }
}

Executor::Executor(const std::string &name_,
                   const std::vector<AsyncRuntime::CPU> &cpus,
                   const std::vector<WorkGroupOption> & work_groups_option)
                   : IExecutor(name_, kCPU_EXECUTOR)
                   , entities_inc{0} {

    std::map<size_t, size_t> cpus_wg = {};
    for (int i = 0; i < cpus.size(); ++i) { cpus_wg[i] = 0; }

    for (int i = 0; i < work_groups_option.size(); ++i) {
        groups.push_back(new ExecutorWorkGroup(i, work_groups_option[i], cpus, cpus_wg));
    }

    main_group = groups[0];
}

Executor::~Executor() {
    for (auto group : groups) {
        delete group;
    }
}

void Executor::MakeMetrics(const std::shared_ptr<Mon::IMetricer> &m) {
    metricer = m;
    if (metricer) {
        for (auto group : groups) {
            group->MakeMetrics(name, metricer);
        }
    }
}

uint16_t Executor::AddEntity(void *ptr) {
    entities_count.fetch_add(1, std::memory_order_relaxed);
    int e = entities_inc.fetch_add(1, std::memory_order_relaxed);
    if (e >= 65500) {
        entities_inc.store(0, std::memory_order_relaxed);
        return 0;
    }
    return e;
}

void Executor::DeleteEntity(uint16_t id) {
    if (entities_count.fetch_sub(1, std::memory_order_relaxed) <= 0) {
        entities_count.store(0, std::memory_order_relaxed);
    }
    for (auto group : groups) {
        group->DeleteEntity(id);
    }
}

void Executor::Post(task *task) {
    auto execute_state = task->get_execution_state();
    execute_state.executor = this;
    task->set_execution_state(execute_state);

    if (execute_state.work_group != INVALID_OBJECT_ID) {
        groups[execute_state.work_group]->Post(task);
    } else {
        main_group->Post(task);
    }
}



