#ifndef AR_EXECUTOR_H
#define AR_EXECUTOR_H

#include "ar/task.hpp"
#include "ar/processor.hpp"
#include "ar/processor_group.hpp"
#include "ar/metricer.hpp"
#include "ar/cpu_helper.hpp"

#ifdef USE_TESTS
class EXECUTOR_TEST_FRIEND;
#endif

namespace AsyncRuntime {

    struct WorkGroupOption {
        std::string                     name;
        double                          cap;
        double                          util;
        int                             slot_concurrency = 0;
    };

    enum ExecutorType {
        kCPU_EXECUTOR,
        kIO_EXECUTOR,
        kUSER_EXECUTOR
    };

#define MAX_ENTITIES 500
#define MAX_GROUPS 10

    class IExecutor: public BaseObject {
    public:
        IExecutor() = default;
        explicit IExecutor(const std::string & name, ExecutorType executor_type);
        ~IExecutor() override = default;

        virtual void Post(task *task) = 0;

        virtual uint16_t AddEntity(void *ptr);

        virtual void DeleteEntity(uint16_t id);

        virtual void SetIndex(int i ) { index = i; }

        virtual void MakeMetrics(const std::shared_ptr<Mon::IMetricer> &m) { metricer = m; }

        int GetEntitiesCount() const { return entities_count.load(std::memory_order_relaxed); }

        int GetMaxEntitiesCount() const { return MAX_ENTITIES; };

        ExecutorType GetType() const { return type; }

        int GetIndex() const { return index; }

        const std::string & GetName() const { return name; }
    protected:
        std::shared_ptr<Mon::IMetricer>                          metricer;
        std::string                                              name;
        ExecutorType                                             type = kUSER_EXECUTOR;
        std::shared_ptr<Mon::Counter>                            m_entities_count;
        std::atomic_int                                          entities_count = {0};
    private:
        int                                                      index = 0;
    };

    class ExecutorSlot;

    class ExecutorWorkGroup {
    public:
        ExecutorWorkGroup(int id,
                          const WorkGroupOption & option,
                          const std::vector<AsyncRuntime::CPU> &cpus,
                          std::map<size_t, size_t>& cpus_wg);
        ~ExecutorWorkGroup();

        void MakeMetrics(const std::string &executor_name, const std::shared_ptr<Mon::IMetricer> &m);

        void Post(task *task);

        void DeleteEntity(uint16_t id);
    private:
        std::shared_ptr<Mon::IMetricer> metricer;
        std::shared_ptr<Mon::Counter>   workers_count;
        std::shared_ptr<Mon::Counter>   slots_count;

        int                             id;
        std::string                     name;
        std::mutex                      mutex;
        std::map<uint16_t, int>         entities_peer_slot;
        std::map<int, int>              cpus_peer_slot;
        std::vector<ExecutorSlot*>      slots;
        std::unique_ptr<Scheduler>      scheduler;
    };

    class Executor  : public IExecutor {
        friend Processor;
#ifdef USE_TESTS
        friend EXECUTOR_TEST_FRIEND;
#endif
    public:
        Executor(const std::string & name_,
                 const std::vector<AsyncRuntime::CPU> & cpus,
                 const std::vector<WorkGroupOption> & work_groups_option);

        ~Executor() override;

        Executor(const Executor&) = delete;
        Executor(Executor&&) = delete;

        Executor& operator =(const Executor&) = delete;
        Executor& operator =(Executor&&) = delete;

        void MakeMetrics(const std::shared_ptr<Mon::IMetricer> &m) override;

        uint16_t AddEntity(void *ptr) override;

        virtual void DeleteEntity(uint16_t id) override;

        void Post(task *task) override;
    private:
        std::atomic_uint16_t                                     entities_inc;
        std::vector<ExecutorWorkGroup*>                          groups;
        ExecutorWorkGroup                                        *main_group;

    };
}

#endif //AR_EXECUTOR_H
