#ifndef AR_EXECUTOR_H
#define AR_EXECUTOR_H

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
        int                             priority = WG_PRIORITY_MEDIUM;
    };

    enum ExecutorType {
        kCPU_EXECUTOR,
        kIO_EXECUTOR,
        kUSER_EXECUTOR
    };

    class IExecutor: public BaseObject {
    public:
        IExecutor() = default;
        explicit IExecutor(const std::string & name, ExecutorType executor_type);

        virtual void Post(Task* task) = 0;

        void IncrementEntitiesCount();

        void DecrementEntitiesCount();

        int GetEntitiesCount() const { return entities_count.load(std::memory_order_relaxed); }

        ExecutorType GetType() const { return type; }

        const std::string & GetName() const { return name; }
    protected:
        std::string                                              name;
        ExecutorType                                             type = kUSER_EXECUTOR;
    private:
        std::shared_ptr<Mon::Counter>                            m_entities_count;
        std::atomic_int                                          entities_count = {0};
    };


    class Executor  : public IExecutor {
        friend Processor;
#ifdef USE_TESTS
        friend EXECUTOR_TEST_FRIEND;
#endif
    public:
        Executor(const std::string & name_,
                 const std::vector<AsyncRuntime::CPU> & cpus,
                 std::vector<WorkGroupOption> work_groups_option = {});

        ~Executor() override;


        Executor(const Executor&) = delete;
        Executor(Executor&&) = delete;


        Executor& operator =(const Executor&) = delete;
        Executor& operator =(Executor&&) = delete;

        /**
         * @brief
         * @param task
         */
        void Post(Task* task) override;


        /**
         * @brief
         * @return
         */
        const std::vector<Processor*>& GetProcessors() const { return processors; }
    private:
        uint                                                     max_processors_count;
        std::shared_ptr<Mon::Counter>                            processors_count;
        std::vector<Processor*>                                  processors;
        std::vector<ProcessorGroup*>                             processor_groups;
        std::vector<WorkGroupOption>                             processor_groups_option;
        ProcessorGroup                                          *main_processor_group;
    };
}

#endif //AR_EXECUTOR_H
