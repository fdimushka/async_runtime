#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;


void async_fun(CoroutineHandler* handler, YieldVoid & yield, Ticker *ticker) {
    yield();

    while (Await(ticker->AsyncTick(), handler)) {
        std::cout << "tick: " << TIMESTAMP_NOW_SEC() << std::endl;
    }
}


int main() {
    SetupRuntime();
    Ticker ticker(1s);
    auto coro = MakeCoroutine(&async_fun, &ticker);
    auto result_async_fun = Async(coro);
    std::this_thread::sleep_for(std::chrono::milliseconds(5100));
    ticker.Stop();
    result_async_fun->Wait();
    Terminate();
    return 0;
}

