#include "tbb_io_executor.h"
#include <signal.h>

#include <utility>
#include "ar/logger.hpp"
#include "ar/profiler.hpp"
#include "uv.h"


using namespace AsyncRuntime;


static uv_async_t exit_handle;

static void SigPIPEHandler(int signal) {
    std::cerr << "Call SIGPIPE" << std::endl;
}

static void ExitAsyncCb(uv_async_t *handle) {
    //uv_close((uv_handle_t*) &exit_handle, nullptr);
    uv_stop((uv_loop_t *) handle->data);
}


void TBBIOExecutor::AsyncIOCb(uv_async_t *handle) {
    if (handle->data != nullptr) {
        auto *executor = (TBBIOExecutor *) handle->data;
        for (;;){
            IOTask *task = nullptr;
            if (executor->rq.try_pop(task)) {
                if (!task->Execute(handle->loop)) {
                    delete task;
                }
            } else {
                break;
            }
        }
    }
}


TBBIOExecutor::TBBIOExecutor(const std::string &name_) : IOExecutor(name_) {
    type = kIO_EXECUTOR;
    loop = uv_default_loop();
    uv_async_init(loop, &exit_handle, ExitAsyncCb);
}


TBBIOExecutor::~TBBIOExecutor() {
    exit_handle.data = loop;
    uv_async_send(&exit_handle);
    loop_thread.Join();
}


void TBBIOExecutor::ThreadRegistration(std::thread::id thread_id) {
//    auto *async = new uv_async_t;
//    async->data = new AsyncHandlerCtx;
//    async_handlers.insert(std::make_pair(thread_id, async));
//    uv_async_init(loop, async, AsyncIOCb);
}


void TBBIOExecutor::Run() {

    async.data = this;
    uv_async_init(loop, &async, &TBBIOExecutor::AsyncIOCb);

    loop_thread.Submit([this] {
        //std::string th_name = ThreadHelper::GetName() + "/io";
        //ThreadHelper::SetName(th_name.c_str());
        PROFILER_ADD_EVENT(1, Profiler::NEW_THREAD);
        Loop();
        PROFILER_ADD_EVENT(1, Profiler::DELETE_THREAD);
    });
}


void TBBIOExecutor::Loop() {
    struct sigaction sh;
    struct sigaction osh;

    sh.sa_handler = &SigPIPEHandler; //Can set to SIG_IGN
    // Restart interrupted system calls
    sh.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigemptyset(&sh.sa_mask);

    if (sigaction(SIGPIPE, &sh, &osh) < 0) {
        std::cerr << "Not set SIGPIPE to sigaction" << std::endl;
        return;
    }
    //ThreadRegistration(loop_thread.GetThreadId());
    uv_run(loop, UV_RUN_DEFAULT);
}


void TBBIOExecutor::Post(Task *task) {
    Post(reinterpret_cast<IOTask *>(task));
}


void TBBIOExecutor::Post(IOTask *io_task) {
//    uv_async_t *handler;
//
//    if (async_handlers.find(std::this_thread::get_id()) != async_handlers.end()) {
//        handler = async_handlers.at(std::this_thread::get_id());
//    } else {
//        handler = async_handlers.at(main_thread_id);
//    }
//
//    auto *ctx = (AsyncHandlerCtx *) handler->data;
//    ctx->run_queue.push(io_task);
//    uv_async_send(handler);
      rq.push(io_task);
      uv_async_send(&async);
}

