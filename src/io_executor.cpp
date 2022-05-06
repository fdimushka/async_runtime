#include "ar/io_executor.hpp"
#include "uv.h"


using namespace AsyncRuntime;


static uv_async_t   exit_handle;


static void ExitAsyncCb(uv_async_t* handle)
{
    uv_close((uv_handle_t*) &exit_handle, nullptr);
}


static void AsyncIOCb(uv_async_t* handle)
{
    if(handle->data != nullptr) {
        auto* ctx = (IOExecutor::AsyncHandlerCtx*)handle->data;
        while(!ctx->run_queue.empty()) {
            auto v = ctx->run_queue.pop();
            if(v) {
                auto *task = (IOTask *) v.value();
                if(!task->Execute(handle->loop)) {
                    delete task;
                }
            }
        }
    }
}


IOExecutor::IOExecutor(const std::string & name_) : name(name_)
{
    loop = uv_default_loop();
    async_handler.data = &async_handler_ctx;

    uv_async_init(loop, &exit_handle, ExitAsyncCb);
    uv_async_init(loop, &async_handler, AsyncIOCb);

    loop_thread.Submit([this] { Loop(); });
}


IOExecutor::~IOExecutor()
{
    uv_async_send(&exit_handle);
    uv_stop(loop);
    loop_thread.Join();
}


void IOExecutor::Loop()
{
    uv_run(loop, UV_RUN_DEFAULT);
}


void IOExecutor::Post(Task *task)
{
    Post(reinterpret_cast<IOTask *>(task));
}


void IOExecutor::Post(IOTask *io_task)
{
    auto* ctx = (AsyncHandlerCtx*)async_handler.data;
    ctx->run_queue.push(io_task);
    uv_async_send(&async_handler);
}

