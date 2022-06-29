#include "ar/ar.hpp"

using namespace AsyncRuntime;


void async_fun(CoroutineHandler* handler, YieldVoid & yield) {
    PROFILER_ADD_EVENT(handler->GetID(), Profiler::REG_ASYNC_FUNCTION);
    yield();
    for(;;) {
        Profiler::State s = Profiler::GetSingletonPtr()->GetCurrentState();
        std::cout << "call async_fun" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        yield();
    }
}


int main() {
    SetupRuntime();
    Coroutine coro = MakeCoroutine(&async_fun);

    for(;;)
        Await(Async(coro));

    Terminate();
    return 0;
}