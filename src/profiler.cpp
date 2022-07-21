#include "ar/profiler.hpp"
#include "ar/runtime.hpp"
#include "ar/os.hpp"

#include "base64.h"

#include "profiler_schema_generated.h"


using namespace AsyncRuntime;
using namespace std::chrono;

#define NO_TS (-1)
#define PROFILER_SERVER_DEFAULT_HOST "127.0.0.1"
#define PROFILER_SERVER_DEFAULT_PORT 9002


inline int64_t Now()
{
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}


void AsyncProfilerLoop(CoroutineHandler *handler, YieldVoid yield, Ticker *ticker) {
    yield();
    while (Await(ticker->AsyncTick(), handler)) {
        Profiler::GetSingletonPtr()->Update();
    }
}


void AsyncGetProfilerState(CoroutineHandler *handler, HTTPConnectionPtr http_connection)
{
    flatbuffers::FlatBufferBuilder  fbb(1024*10);
    std::vector<flatbuffers::Offset<AsyncRuntime::WorkSchema>> fb_works;
    std::vector<uint64_t> fb_threads;
    Profiler::State state = Profiler::GetSingletonPtr()->GetCurrentState();

    for(const auto& work_it : state.work_ground) {
        const auto &work = work_it.second;
        std::vector<WorkStepSchema> fb_work_time;

        for(const auto& work_step : work.work_time) {
            fb_work_time.emplace_back(WorkStepSchema(work_step.thread, work_step.begin, work_step.end));
        }

        fb_works.emplace_back(
                CreateWorkSchema(fbb,
                         work_it.first,
                         fbb.CreateString(work.name),
                         fbb.CreateVectorOfStructs(fb_work_time),
                         work.begin_at,
                         work.updated_at));
    }

    for(auto thread_it : state.threads) {
        fb_threads.emplace_back(thread_it);
    }

    auto fb_state = CreateStateSchema(fbb,
                                      fbb.CreateString(state.app_info),
                                      fbb.CreateString(state.system_info),
                                      state.profiling_interval,
                                      state.created_at,
                                      state.coroutines_count,
                                      fbb.CreateVector(fb_works),
                                      fbb.CreateVector(fb_threads));

    fbb.Finish(fb_state);
    http_connection->SetAccessAllowOrigin("*");
    http_connection->AsyncResponse(HTTPStatus::OK, (const char*)fbb.GetBufferPointer(), fbb.GetSize(), "application/octet-stream");
}


Profiler::WorkTimeEstimator::WorkTimeEstimator(uintptr_t id) : work_id(id), begin_ts(Now())
{
}


Profiler::WorkTimeEstimator::~WorkTimeEstimator()
{
    if(work_id > 0)
        Profiler::GetSingletonPtr()->AddEvent(work_id, WORK_TIME_STEP, begin_ts, Now());
}


Profiler::State::State(Profiler::State &other)
{
    created_at = other.created_at;
    threads = other.threads;
    work_ground = other.work_ground;
    coroutines_count = other.coroutines_count;
    system_info = other.system_info;
    profiling_interval = other.profiling_interval;
    app_info = other.app_info;
}


Profiler::State &Profiler::State::operator=(Profiler::State other)
{
    created_at = other.created_at;
    threads = other.threads;
    work_ground = other.work_ground;
    coroutines_count = other.coroutines_count;
    system_info = other.system_info;
    profiling_interval = other.profiling_interval;
    app_info = other.app_info;
    return *this;
}


Profiler::Profiler() :
    profiler_server_port(PROFILER_SERVER_DEFAULT_PORT),
    profiler_server_host(PROFILER_SERVER_DEFAULT_HOST),
    events(2048),
    ticker(5s) {
    state.profiling_interval = Timestamp::CastMicro(5min);
    state.system_info = "CPU: " + OS::GetCPUInfo();
}


Profiler::~Profiler()
{
    ticker.Stop();
    if(result && coroutine) {
        result->Wait();
        coroutine.reset();
    }
}


void Profiler::SetAppInfo(int argc, char *argv[])
{
    std::string app_str;
    for (int i = 0; i < argc; ++i)
        app_str += std::string(argv[i]) + " ";

    std::lock_guard<std::mutex> lock(mutex);
    state.app_info = app_str;
}


void Profiler::SetServerPort(int port)
{
    profiler_server_port = port;
}


void Profiler::SetServerHost(const std::string &host)
{
    profiler_server_host = host;
}


void Profiler::Start()
{
    state.created_at = Now();
    profiler_server.AddRoute("/profiler/state", GET, &AsyncGetProfilerState);
    //profiler_server.AddRoute("/profiler", GET, &AsyncGetProfilerHTML);
    coroutine = std::make_shared<Coroutine<void>>(&AsyncProfilerLoop, &ticker);
    result = Async(*coroutine);
    profiler_server.AsyncBind(profiler_server_host, profiler_server_port, [this](){
        AR_LOG_SS(Info, "Profiler server started on: "
        << "http://" << profiler_server_host << ":" << profiler_server_port << "/profiler/state");
    }, [this](int error){
        AR_LOG_SS(Error, "Profiler server bind error: " << FSErrorMsg(error));
    });
}


void Profiler::Stop()
{
    ticker.Stop();
    if(result && coroutine) {
        result->Wait();
        coroutine.reset();
    }
}


void Profiler::Update()
{
    std::vector<Event*> new_events;
    size_t events_size = events.size();

    for(size_t i = 0; i < events_size; ++i){
        auto v = events.pop();
        if(v) {
            new_events.push_back(v.value());
        }
    }

    {
        std::reverse(new_events.begin(), new_events.end());
        std::lock_guard<std::mutex> lock(mutex);
        for(auto *event : new_events) {
            if(event->type == EventType::WORK_TIME_STEP) {
                auto work_it = state.work_ground.find(event->id);
                if (work_it != state.work_ground.end()) {
                    auto &work = work_it->second;
                    work.work_time.push_back(WorkStep{
                            event->thread,
                            event->begin_ts,
                            event->end_ts
                    });
                    work.updated_at = Now();

                    if (work.begin_at <= -1)
                        work.begin_at = event->begin_ts;

                    if (work.updated_at > 0 && work.begin_at > 0) {
                        while (!work.work_time.empty() &&
                               work.updated_at - work.begin_at > state.profiling_interval) {
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


void Profiler::AddEvent(uintptr_t id, EventType type, Timespan begin_ts, Timespan end_ts)
{
    auto *event = new Event{
            id,
            type,
            std::hash<std::thread::id>{}(std::this_thread::get_id()),
            begin_ts,
            end_ts
    };

    events.push(event);
}


void Profiler::AddEvent(uintptr_t id, EventType type)
{
    auto *event = new Event{
        id,
        type,
        std::hash<std::thread::id>{}(std::this_thread::get_id()),
        Now(),
        NO_TS
    };

    events.push(event);
}


Profiler::State Profiler::GetCurrentState()
{
    std::lock_guard<std::mutex> lock(mutex);
    return state;
}
