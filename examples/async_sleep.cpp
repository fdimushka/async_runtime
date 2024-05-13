#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;


void async_fun(coroutine_handler* handler, yield<void> & yield) {
    std::cout << "call async sleep" << std::endl;
    auto t_start = Timestamp::Now();
    Await(AsyncSleep(1s), handler);
    Await(AsyncSleep(1s), handler);
    Await(AsyncSleep(1s), handler);
    auto t_end = Timestamp::Now();
    std::cout << "end sleep: " << t_end - t_start << "ms" << std::endl;
}


int main() {
    SetupRuntime();
    auto coro = make_coroutine(&async_fun);
    Await(Async(coro));
    Terminate();
    return 0;
}

