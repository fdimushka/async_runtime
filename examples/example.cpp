#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;





class Stream {
public:
    explicit Stream(int index) :
        ticker(25ms),
        coro_process(MakeCoroutine(&Stream::process_fun, &ticker, index))
        {
            res_process = Async(coro_process);
        }


    void Wait() {
        res_process->Wait();
    }

    static void process_fun(CoroutineHandler* handler, YieldVoid & yield, Ticker *ticker, int index) {
        yield();

        auto t_start = std::chrono::high_resolution_clock::now();
        while (Await(ticker->AsyncTick(), handler)) {
            auto t_end = std::chrono::high_resolution_clock::now();
            if(index == 100) {
                double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
                std::cout << "tick: " << elapsed_time_ms << " " << std::this_thread::get_id() << std::endl;
            }

            t_start = std::chrono::high_resolution_clock::now();
            //std::cout << "tick: " << TIMESTAMP_NOW_SEC() << std::endl;
        }
    }
private:
    Ticker ticker;
    std::shared_ptr<Result<void>>    res_process;
    Coroutine<void> coro_process;
};


int main() {
    SetupRuntime();
    int streams_size = 300;
    std::vector<std::shared_ptr<Stream>> streams;
    streams.reserve(streams_size);

    for(int i = 0; i < streams_size; ++i) {
        //std::this_thread::sleep_for(100ms);
        streams.push_back(std::make_shared<Stream>(i));
    }

    for(int i = 0; i < streams_size; ++i) {
        streams[i]->Wait();
    }
//
//    auto result_async_fun = Async(coro);
//    std::this_thread::sleep_for(std::chrono::milliseconds(5100));
//    ticker.Stop();
//    result_async_fun->Wait();
    return 0;
}

