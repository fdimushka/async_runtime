#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;


void async_fun(CoroutineHandler* handler, YieldVoid & yield) {
    yield();

    std::cout << "call async sleep" << std::endl;
    auto t_start = Timestamp::Now();
    Await(AsyncSleep(1s), handler);
    auto t_end = Timestamp::Now();
    std::cout << "end sleep: " << t_end - t_start << "ms" << std::endl;
}


int main() {
    SetupRuntime();
    auto coro = MakeCoroutine(&async_fun);
    Await(Async(coro));
    Terminate();
    return 0;
}

