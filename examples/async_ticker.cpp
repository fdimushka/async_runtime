#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;


void async_fun(CoroutineHandler* handler, YieldVoid & yield) {
    yield();

    Ticker ticker(1s);
    while (Await(ticker.AsyncTick(), handler)) {
        std::cout << "tick: " << TIMESTAMP_NOW_SEC() << std::endl;
    }
}


int main() {
    SetupRuntime();
    auto coro = MakeCoroutine(&async_fun);
    Await(Async(coro));
    Terminate();
    return 0;
}

