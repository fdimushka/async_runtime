#include "ar/io_executor.hpp"

#include <utility>
#include "ar/logger.hpp"
#include "ar/profiler.hpp"
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
            auto v = ctx->run_queue.steal();
            if(v) {
                auto *task = (IOTask *) v.value();
                if(!task->Execute(handle->loop)) {
                    delete task;
                }
            }
        }
    }
}


IOExecutor::IOExecutor(std::string  name_) : name(std::move(name_))
{
    type = kIO_EXECUTOR;
    loop = uv_default_loop();
    uv_async_init(loop, &exit_handle, ExitAsyncCb);

    main_thread_id = std::this_thread::get_id();
    ThreadRegistration(main_thread_id);
}


IOExecutor::~IOExecutor()
{
    uv_async_send(&exit_handle);
    uv_stop(loop);
    loop_thread.Join();

    for(const auto & async : async_handlers) {
        delete (AsyncHandlerCtx *)async.second->data;
        delete async.second;
    }

    async_handlers.clear();
}


void IOExecutor::ThreadRegistration(std::thread::id thread_id)
{
    auto *async = new uv_async_t;
    async->data = new AsyncHandlerCtx;
    async_handlers.insert(std::make_pair(thread_id, async));
    uv_async_init(loop, async, AsyncIOCb);
}


void IOExecutor::Run()
{
    loop_thread.Submit([this] {
        //std::string th_name = ThreadHelper::GetName() + "/io";
        //ThreadHelper::SetName(th_name.c_str());
        PROFILER_ADD_EVENT(1, Profiler::NEW_THREAD);
        Loop();
        PROFILER_ADD_EVENT(1, Profiler::DELETE_THREAD);
    });
}


void IOExecutor::Loop()
{
    ThreadRegistration(loop_thread.GetThreadId());
    uv_run(loop, UV_RUN_DEFAULT);
}


void IOExecutor::Post(Task *task)
{
    Post(reinterpret_cast<IOTask *>(task));
}


void IOExecutor::Post(IOTask *io_task)
{
    uv_async_t *handler;

    if(async_handlers.find(std::this_thread::get_id()) != async_handlers.end()) {
        handler = async_handlers.at(std::this_thread::get_id());
    }else{
        handler = async_handlers.at(main_thread_id);
    }

    auto* ctx = (AsyncHandlerCtx*)handler->data;
    ctx->run_queue.push(io_task);
    uv_async_send(handler);
}

