#include <iostream>
#include "ar/ar.hpp"
#include "ar/tbb_executor.hpp"
#include "ar/timestamp.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;

struct ctx {

};

static void async_loop(CoroutineHandler *handler, YieldVoid & yield) {

    Ticker ticker(100ms);
    yield();

    int i = 0;
    while (Await(ticker.AsyncTick(handler), handler)) {
        AR_LOG_SS(Info, "async_loop " << std::this_thread::get_id());
        i++;
        if (i > 10) {
            break;
        }
    }
    ticker.Stop();
}

int main() {

    AsyncRuntime::Logger::s_logger.SetStd();
    AsyncRuntime::SetupRuntime();

    {
        ctx c;
        auto tag = AddEntityTag(&c);
        auto coro = AsyncRuntime::MakeCoroutine(async_loop);
        coro.SetEntityTag(tag);
        Await(Async(coro));
        DeleteEntityTag(tag);
    }

  return 0;
}

