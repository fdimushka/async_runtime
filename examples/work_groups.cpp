#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;

void async_management_fun(CoroutineHandler* handler, YieldVoid & yield, Ticker *ticker) {
    yield();

    while (Await(ticker->AsyncTick(handler), handler)) {
        std::cout << handler->GetExecutorState().work_group << " " << handler->GetExecutorState().processor << " tick management: " << TIMESTAMP_NOW() << std::endl;
    }
}

void async_va_fun(CoroutineHandler* handler, YieldVoid & yield, Ticker *ticker) {
    yield();

    while (Await(ticker->AsyncTick(handler), handler)) {
        std::cout << handler->GetExecutorState().work_group << " " << handler->GetExecutorState().processor << " tick va: " << TIMESTAMP_NOW() << std::endl;
    }
}

int main() {
    SetupRuntime({
        {
                             {"management", 1.0, 1.0, WG_PRIORITY_MEDIUM},
                             {"va", 1.0, 1.0, WG_PRIORITY_HIGH}
        }
    });

    auto management_group = GetWorkGroup("management");
    auto va_group = GetWorkGroup("va");

    Ticker ticker_management(100ms);
    Ticker ticker_va(1s);
    auto coro_management = MakeCoroutine(&async_management_fun, &ticker_management);
    auto coro_va = MakeCoroutine(&async_va_fun, &ticker_va);

    coro_management.SetWorkGroup(management_group);
    coro_va.SetWorkGroup(va_group);

    auto f1 = Async(coro_management);
    auto f2 = Async(coro_va);

    f1->Wait();
    f2->Wait();

    ticker_management.Stop();
    ticker_va.Stop();
    Terminate();

    return 0;
}

