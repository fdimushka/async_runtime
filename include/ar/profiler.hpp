#ifndef AR_PROFILER_H
#define AR_PROFILER_H


#include "ar/task.hpp"
#include "ar/ticker.hpp"
#include "ar/work_steal_queue.hpp"
#include "ar/coroutine.hpp"
#include "ar/http.hpp"

#include <list>


namespace AsyncRuntime {
    /**
     * @brief
     * @class Profiler
     */
    class Profiler {
    public:
        struct WorkTimeEstimator {
            uintptr_t work_id;
            int64_t begin_ts;
            explicit WorkTimeEstimator(uintptr_t id);
            ~WorkTimeEstimator();
        };


        enum EventType: uint8_t {
            WORK_TIME_STEP,
            NEW_COROUTINE,
            DELETE_COROUTINE,
            NEW_THREAD,
            DELETE_THREAD,
            REG_ASYNC_FUNCTION
        };


        struct WorkStep {
            uint64_t thread;
            int64_t begin;
            int64_t end;
        };


        struct Work {
            uintptr_t id;
            std::string name;
            std::list<WorkStep>   work_time;
            int64_t begin_at = -1;
            int64_t updated_at = -1;
        };


        struct State {
            std::string                             system_info;
            int64_t                                 coroutines_count = 0;
            int64_t                                 created_at = 0;
            std::unordered_map<uintptr_t, Work>     work_ground;
            std::list<uint64_t>                     threads;

            State() = default;
            State(State &&other) = delete;
            State(State &other);

            State& operator=(State other);
        };


        Profiler();
        ~Profiler() = default;


        void Start();
        void Stop();
        void Update();


        /**
         * @brief
         * @param id
         * @param name
         */
        void RegAsyncFunction(uintptr_t id, const char* name);


        /**
         * @brief
         * @param id
         * @param type
         */
        void AddEvent(uintptr_t id, EventType type);


        /**
         * @brief
         * @param id
         * @param type
         * @param begin_ts
         * @param end_ts
         */
        void AddEvent(uintptr_t id, EventType type, Timespan begin_ts, Timespan end_ts);


        /**
         * @brief
         * @return
         */
        State GetCurrentState();


        static Profiler* GetSingletonPtr() {
            static Profiler instance;
            return &instance;
        }
    private:

        struct Event {
            uintptr_t id;
            EventType type;
            uint64_t thread;
            int64_t begin_ts;
            int64_t end_ts;
        };


        HttpServer                                      profiler_server;
        std::shared_ptr<Coroutine<void>>                coroutine;
        ResultVoidPtr                                   result;
        std::mutex                                      mutex;
        State                                           state;
        WorkStealQueue<Event*>                          events;
        Ticker                                          ticker;
        int64_t                                         time_interval;
    };


#if defined(USE_PROFILER)
#define PROFILER_START() Profiler::GetSingletonPtr()->Start();
#define PROFILER_STOP() Profiler::GetSingletonPtr()->Stop();
#define PROFILER_ADD_EVENT(ID, TYPE)                                                \
    if((ID) > 0) {                                                                  \
        if(TYPE != Profiler::REG_ASYNC_FUNCTION) {                                  \
            Profiler::GetSingletonPtr()->AddEvent(ID, TYPE);                        \
        }else{                                                                      \
            Profiler::GetSingletonPtr()->RegAsyncFunction(ID, __PRETTY_FUNCTION__); \
        }                                                                           \
    };                                                                              \

#define PROFILER_TASK_WORK_TIME(ID) Profiler::WorkTimeEstimator  estimator(ID);
#define PROFILER_REG_ASYNC_FUNCTION(ID) PROFILER_ADD_EVENT(ID, Profiler::REG_ASYNC_FUNCTION)

#else
#define PROFILER_START()
#define PROFILER_STOP()
#define PROFILER_ADD_EVENT(ID, TYPE)
#define PROFILER_TASK_WORK_TIME(ID)
#define PROFILER_REG_ASYNC_FUNCTION(ID)
#endif

}

#endif //AR_PROFILER_H
