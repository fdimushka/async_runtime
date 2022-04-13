#ifndef AR_IO_EXECUTOR_H
#define AR_IO_EXECUTOR_H


#include "ar/executor.hpp"
#include "ar/io_task.hpp"
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
         * @param handler
         */
        void RegistrationAsyncHandler(uv_async_t* handler);


        /**
         * @brief
         */
        void Run();


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


        std::condition_variable             cv;
        ThreadExecutor                      loop_thread;
        std::string                         name;
        uv_loop_t                           *loop;
        std::vector<uv_async_t *>           async_handlers;
        static uv_async_t                   main_async_io_handle;
    };
}

#endif //AR_IO_EXECUTOR_H
