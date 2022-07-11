#include "ar/profiler.hpp"
#include "ar/runtime.hpp"


using namespace AsyncRuntime;
using namespace std::chrono;

#define NO_TS (-1)


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


Profiler::State::State(Profiler::State &other)
{
    work_ground = other.work_ground;
    coroutines_count = other.coroutines_count;
}


Profiler::State &Profiler::State::operator=(Profiler::State other)
{
    coroutines_count = other.coroutines_count;
    return *this;
}


Profiler::Profiler() :
    ticker(5s),
    time_interval(Timestamp::CastMicro(5min)) { }


void Profiler::Start()
{
    coroutine = std::make_shared<Coroutine<void>>(&AsyncProfilerLoop, &ticker);
    result = Async(*coroutine);
}


void Profiler::Stop()
{
    ticker.Stop();
    if(result)
        result->Wait();
}


void Profiler::Update()
{
    size_t events_count = events.size();
    std::vector<Event*> new_events;

    for(size_t i = 0; i < events_count; ++i) {
        auto v = events.pop();
        if(v) {
            new_events.push_back(v.value());
        }
    }

    {
        std::reverse(new_events.begin(), new_events.end());
        std::lock_guard<std::mutex> lock(mutex);
        for(auto *event : new_events) {

            if(event->type == EventType::BEGIN_WORK || event->type == EventType::END_WORK) {
                auto work_it = state.work_ground.find(event->id);
                if (work_it != state.work_ground.end()) {
                    auto &work = work_it->second;
                    if (event->type == EventType::BEGIN_WORK) {
                        work.work_time.push_back(WorkStep{
                                event->thread,
                                event->ts,
                                NO_TS
                        });
                        work.updated_at = Now();

                        if (work.begin_at <= -1)
                            work.begin_at = event->ts;

                    } else if (event->type == EventType::END_WORK) {
                        if (!work.work_time.empty()) {
                            work.work_time.back().end = event->ts;
                            work.updated_at = Now();
                        }
                    }

                    if (work.updated_at > 0 && work.begin_at > 0) {
                        while (!work.work_time.empty() &&
                               work.updated_at - work.begin_at > time_interval) {
                            work.work_time.pop_front();

                            if (!work.work_time.empty())
                                work.begin_at = work.work_time.front().begin;
                        }
                    }
                }
            } else if (event->type == EventType::NEW_COROUTINE) {
                state.coroutines_count++;
            } else if (event->type == EventType::DELETE_COROUTINE) {
                state.coroutines_count--;
                if (state.coroutines_count < 0) state.coroutines_count = 0;
            } else if(event->type == EventType::NEW_THREAD) {
                state.threads.push_back(event->thread);
            } else if(event->type == EventType::DELETE_THREAD) {
                state.threads.remove(event->thread);
            }

            delete event;
        }
    }
}


void Profiler::RegAsyncFunction(uintptr_t id, const char *name)
{
    std::lock_guard<std::mutex> lock(mutex);
    state.work_ground.insert(std::make_pair(id, std::move(Work{
            id,
            {name},
            { },
            Now()
    })));
}


void Profiler::AddEvent(uintptr_t id, EventType type)
{
    auto *event = new Event{
        id,
        type,
        std::hash<std::thread::id>{}(std::this_thread::get_id()),
        Now()
    };

    events.push(event);
}


Profiler::State Profiler::GetCurrentState()
{
    std::lock_guard<std::mutex> lock(mutex);
    return state;
}