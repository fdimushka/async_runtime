#include "ar/profiler.hpp"


using namespace AsyncRuntime;
using namespace std::chrono;


inline int64_t Now()
{
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
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
    Event event{
        id,
        type,
        std::hash<std::thread::id>{}(std::this_thread::get_id()),
        Now()
    };

    events.push(event);
}