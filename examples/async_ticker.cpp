#include <iostream>
#include <chrono>
#include "ar/ar.hpp"
#include "ar/logger.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;


void async_fun(coroutine_handler* handler, yield<void> & yield, Ticker *ticker) {
    auto t_start = std::chrono::high_resolution_clock::now();

    while (Await(ticker->AsyncTick(), handler)) {
        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        AR_LOG_SS(Info, "tick: " << elapsed_time_ms);

        t_start = std::chrono::high_resolution_clock::now();
    }
}


int main() {
    AsyncRuntime::Logger::s_logger.SetStd();
    SetupRuntime();
    Ticker ticker(40ms);
    auto coro = make_coroutine(&async_fun, &ticker);
    auto result_async_fun = Async(coro);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    ticker.Stop();
    result_async_fun.wait();
    Terminate();
    return 0;
}

