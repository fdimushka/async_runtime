#ifndef AR_IO_EXECUTOR_H
#define AR_IO_EXECUTOR_H


#include "ar/executor.hpp"
#include "ar/io_task.h"
#include "uv.h"


namespace AsyncRuntime {


    class IOExecutor : public IExecutor {
    public:
        explicit IOExecutor(const std::string & name_);
        ~IOExecutor() override;


        IOExecutor(const IOExecutor&) = delete;
        IOExecutor(IOExecutor&&) = delete;
        IOExecutor& operator =(const IOExecutor&) = delete;
        IOExecutor& operator =(IOExecutor&&) = delete;


        /**
         * @brief
         * @param task
         */
        void Post(Task* task) override;
    private:
        void Loop();

        std::condition_variable     cv;
        std::mutex                  fs_mutex;
        ThreadExecutor              loop_thread;
        std::string                 name;
        uv_loop_t                   *loop;
    };
}

#endif //AR_IO_EXECUTOR_H
