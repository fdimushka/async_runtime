#include <benchmark/benchmark.h>
#include "ar/ar.hpp"

namespace AR = AsyncRuntime;


void async_func(AR::CoroutineHandler* handler, AR::YieldVoid & yield, benchmark::State& state) {
    for(;;) {
        yield();
    }
}


void async_task(benchmark::State& state) {
    state.PauseTiming();
}


void async_empty_task(benchmark::State& state) { }


static void context_switch(benchmark::State& state) {
    AR::Coroutine coro = AR::MakeCoroutine(&async_func, state);

    for (auto _ : state) {
        coro();
    }
}


static void concurrent_task_call(benchmark::State& state) {
    AR::SetupRuntime();

    for (auto _ : state) {
        state.ResumeTiming();
        AR::Await(AR::Async(&async_task, state));
    }
}


static void await_task(benchmark::State& state) {
    AR::SetupRuntime();

    for (auto _ : state) {
        AR::Await(AR::Async(&async_empty_task, state));
    }
}


// Register the function as a benchmark
BENCHMARK(context_switch);
BENCHMARK(concurrent_task_call);
BENCHMARK(await_task);

// Run the benchmark
BENCHMARK_MAIN();