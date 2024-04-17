#ifndef TEYE_STREAMSERVER_TBB_IO_EXECUTOR_H
#define TEYE_STREAMSERVER_TBB_IO_EXECUTOR_H


#include "ar/io_executor.hpp"
#include "ar/io_task.hpp"
#include "uv.h"
#include <oneapi/tbb.h>

namespace AsyncRuntime {


    class TBBIOExecutor : public IOExecutor {
        
    public:
        explicit TBBIOExecutor(const std::string & name_);
        ~TBBIOExecutor() override;


        TBBIOExecutor(const TBBIOExecutor&) = delete;
        TBBIOExecutor(TBBIOExecutor&&) = delete;
        TBBIOExecutor& operator =(const TBBIOExecutor&) = delete;
        TBBIOExecutor& operator =(TBBIOExecutor&&) = delete;


        /**
         * @brief
         */
        void Run() final;


        /**
         * @brief
         * @param thread_id
         */
        void ThreadRegistration(std::thread::id thread_id) final;


        /**
         * @brief
         * @param task
         */
        void Post(Task* task) final;


        /**
         * @brief
         * @tparam Method
         * @param task
         */
        void Post(IOTask *io_task) final;
    private:
        static void AsyncIOCb(uv_async_t *handle);
        void Loop() final;


        uv_async_t async;
        oneapi::tbb::concurrent_queue<IOTask*>              rq;
        ThreadExecutor                                      loop_thread;
        uv_loop_t                                           *loop;
    };
}


#endif //TEYE_STREAMSERVER_TBB_IO_EXECUTOR_H
