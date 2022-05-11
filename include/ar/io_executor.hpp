#ifndef AR_IO_EXECUTOR_H
#define AR_IO_EXECUTOR_H


#include "ar/executor.hpp"
#include "ar/io_task.hpp"
#include "uv.h"


namespace AsyncRuntime {


    class IOExecutor : public IExecutor {
    public:
        struct AsyncHandlerCtx {
            WorkStealQueue<IOTask *>       run_queue;
        };

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


        /**
         * @brief
         * @tparam Method
         * @param task
         */
        void Post(IOTask *io_task);
    private:
        void Loop();


        ThreadExecutor                      loop_thread;
        std::string                         name;
        uv_loop_t                           *loop;
        uv_async_t                          async_handler;
        AsyncHandlerCtx                     async_handler_ctx;
    };
}

#endif //AR_IO_EXECUTOR_H
