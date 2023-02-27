#ifndef AR_EXECUTOR_H
#define AR_EXECUTOR_H

#include "ar/processor.hpp"
#include "ar/processor_group.hpp"

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


    class IExecutor: public BaseObject {
    public:
        virtual void Post(Task* task) = 0;
    };


    class Executor  : public IExecutor {
        friend Processor;
#ifdef USE_TESTS
        friend EXECUTOR_TEST_FRIEND;
#endif
    public:
        explicit Executor(std::string  name_,
                          std::vector<WorkGroupOption> work_groups_option = {},
                          uint max_processors_count_ = std::thread::hardware_concurrency());
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
        std::vector<Processor*>                                  processors;
        std::vector<ProcessorGroup*>                             processor_groups;
        std::vector<WorkGroupOption>                             processor_groups_option;
        std::string                                              name;
        ProcessorGroup                                          *main_processor_group;
    };
}

#endif //AR_EXECUTOR_H
