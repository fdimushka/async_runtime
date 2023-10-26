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
         */
        void Run();


        /**
         * @brief
         * @param thread_id
         */
        void ThreadRegistration(std::thread::id thread_id);


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


        std::unordered_map<std::thread::id, uv_async_t*>    async_handlers;
        ThreadExecutor                                      loop_thread;
        uv_loop_t                                           *loop;
        std::thread::id                                     main_thread_id;
    };
}

#endif //AR_IO_EXECUTOR_H
