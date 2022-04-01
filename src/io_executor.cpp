#include "ar/io_executor.hpp"
#include "uv.h"


using namespace AsyncRuntime;


static uv_async_t   exit_handle;


static void ExitAsyncCb(uv_async_t* handle)
{
    uv_close((uv_handle_t*) &exit_handle, NULL);
}


IOExecutor::IOExecutor(const std::string & name_) : name(name_)
{
    loop_thread.Submit([this] { Loop(); });
}


IOExecutor::~IOExecutor()
{
    uv_async_send(&exit_handle);
    loop_thread.Join();
}


void IOExecutor::Loop()
{
    loop = uv_loop_new();
    uv_async_init(loop, &exit_handle, ExitAsyncCb);
    uv_run(loop, UV_RUN_DEFAULT);
}


void IOExecutor::Post(Task *task)
{
    std::lock_guard<std::mutex> lock(fs_mutex);
    ExecutorState state;
    state.executor = this;
    state.data = loop;
    task->Execute(state);
}

