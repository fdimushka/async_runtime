#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono;


void async_fun_a(CoroutineHandler* handler, YieldVoid & yield) {
    PROFILER_REG_ASYNC_FUNCTION(handler->GetID());
    yield();
    for(;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        yield();
    }
}


void async_fun_b(CoroutineHandler* handler, YieldVoid & yield) {
    PROFILER_REG_ASYNC_FUNCTION(handler->GetID());
    yield();
    for(;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        yield();
    }
}


int main(int argc, char *argv[]) {
    LOGGER_SET_STD();
    PROFILER_SET_APP_INFO(argc, argv);
    PROFILER_SET_PROFILING_INTERVAL(5min);
    PROFILER_SET_SERVER_HOST("0.0.0.0");
    PROFILER_SET_SERVER_PORT(9002);

    SetupRuntime();
    Coroutine coro_a = MakeCoroutine(&async_fun_a);
    Coroutine coro_b = MakeCoroutine(&async_fun_b);

    for(;;) {
        auto r_a = Async(coro_a);
        auto r_b = Async(coro_b);

        r_a->Wait();
        r_b->Wait();
    }

    Terminate();
    return 0;
}