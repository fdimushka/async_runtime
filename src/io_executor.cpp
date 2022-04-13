#include "ar/io_executor.hpp"
#include "uv.h"


using namespace AsyncRuntime;


static uv_async_t   exit_handle;
uv_async_t   IOExecutor::main_async_io_handle;


static void ExitAsyncCb(uv_async_t* handle)
{
    uv_close((uv_handle_t*) &exit_handle, nullptr);
}


static void AsyncIOCb(uv_async_t* handle)
{
    if(handle->data != nullptr) {
        auto *task = (IOTask *) handle->data;
        task->Execute(handle->loop);
        handle->data = nullptr;
    }
}


IOExecutor::IOExecutor(const std::string & name_) : name(name_)
{
    async_handlers.push_back(&main_async_io_handle);
    loop = uv_default_loop();
}


IOExecutor::~IOExecutor()
{
    uv_async_send(&exit_handle);
    uv_stop(loop);
    loop_thread.Join();
}


void IOExecutor::Run()
{
    uv_async_init(loop, &exit_handle, ExitAsyncCb);
    for(auto *handler : async_handlers)
        uv_async_init(loop, handler, AsyncIOCb);

    loop_thread.Submit([this] { Loop(); });
}


void IOExecutor::Loop()
{
    uv_run(loop, UV_RUN_DEFAULT);
}


void IOExecutor::RegistrationAsyncHandler(uv_async_t *handler)
{
    async_handlers.push_back(handler);
}


void IOExecutor::Post(Task *task)
{
    Post(reinterpret_cast<IOTask *>(task));
}


void IOExecutor::Post(IOTask *io_task)
{
    if(io_task->async_handle != nullptr) {
        io_task->async_handle->data = io_task;
        uv_async_send(io_task->async_handle);
    }else{
        main_async_io_handle.data = io_task;
        uv_async_send(&main_async_io_handle);
    }
}

