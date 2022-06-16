#include "ar/profiler.hpp"
#include "ar/runtime.hpp"


using namespace AsyncRuntime;
using namespace std::chrono;


inline int64_t Now()
{
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}


static void AsyncProfilerLoop(CoroutineHandler *handler, YieldVoid yield, Ticker *ticker) {
    yield();
    while (Await(ticker->AsyncTick(), handler)) {
        Profiler::GetSingletonPtr()->Update();
    }
}


Profiler::Profiler() :
    ticker(1s)
    , coroutine(MakeCoroutine(&AsyncProfilerLoop, &ticker)) { }


void Profiler::Start()
{
    result = Async(coroutine);
}


void Profiler::Stop()
{
    ticker.Stop();
    if(result)
        result->Wait();
}


void Profiler::Update()
{

}


void Profiler::RegWork(uintptr_t id, const char *name)
{
    std::lock_guard<std::mutex> lock(mutex);
    work_ground.insert(std::make_pair(id, std::move(Work{
            id,
            {name},
            { },
            Now()
    })));
}


void Profiler::AddWorkEvent(uintptr_t id, uint8_t type)
{
    auto *event = new Event{
        id,
        type,
        std::hash<std::thread::id>{}(std::this_thread::get_id()),
        Now()
    };

    events.push(event);
}