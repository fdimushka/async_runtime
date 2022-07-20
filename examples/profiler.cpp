#include "ar/ar.hpp"

using namespace AsyncRuntime;


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


int main() {
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