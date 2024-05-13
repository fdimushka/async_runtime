#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;

void async_management_fun(CoroutineHandler* handler, YieldVoid & yield, Ticker *ticker) {

    while (Await(ticker->AsyncTick(handler), handler)) {
        std::cout << handler->get_execution_state().work_group << " " << handler->get_execution_state().processor << " tick management: " << TIMESTAMP_NOW() << std::endl;
    }
}

void async_va_fun(CoroutineHandler* handler, YieldVoid & yield, Ticker *ticker) {

    while (Await(ticker->AsyncTick(handler), handler)) {
        std::cout << handler->get_execution_state().work_group << " " << handler->get_execution_state().processor << " tick va: " << TIMESTAMP_NOW() << std::endl;
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
    auto coro_management = make_coroutine(&async_management_fun, &ticker_management);
    auto coro_va = make_coroutine(&async_va_fun, &ticker_va);

    task::execution_state state_1, state_2;
    state_1.work_group = management_group;
    state_2.work_group = va_group;
    coro_management->set_execution_state(state_1);
    coro_va->set_execution_state(state_2);

    auto f1 = Async(coro_management);
    auto f2 = Async(coro_va);

    f1.wait();
    f2.wait();

    ticker_management.Stop();
    ticker_va.Stop();
    Terminate();

    return 0;
}

