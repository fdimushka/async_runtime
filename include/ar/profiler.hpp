#ifndef AR_PROFILER_H
#define AR_PROFILER_H


#include "ar/task.hpp"
#include "ar/ticker.hpp"
#include "ar/work_steal_queue.hpp"


namespace AsyncRuntime {


#define PROFILER_BEGIN_WORK_EVENT 0
#define PROFILER_END_WORK_EVENT 1

    /**
     * @brief
     * @class Profiler
     */
    class Profiler {
    public:
        Profiler();
        ~Profiler() = default;


        /**
         * @brief
         * @param id
         * @param name
         */
        void RegWork(uintptr_t id, const char* name);


        /**
         * @brief
         * @param id
         * @param type
         */
        void AddWorkEvent(uintptr_t id, uint8_t type);


        static Profiler* GetSingletonPtr() {
            static Profiler instance;
            return &instance;
        }
    private:

        struct Event {
            uintptr_t id;
            uint8_t type;
            uint64_t cpu;
            int64_t ts;
        };


        struct WorkStep {
            uint64_t cpu;
            int64_t begin;
            int64_t end;
        };


        struct Work {
            uintptr_t id;
            std::string name;
            std::vector<WorkStep>   work_time;
            int64_t updated_at;
        };


        std::mutex                                      mutex;
        std::unordered_map<uintptr_t, Work>             work_ground;
        WorkStealQueue<Event>                           events;
        Ticker                                          ticker;
    };


//#define PROFILE_BEGIN_TASK(TASK) Profiler::GetSingletonPtr()->BeginTask(TASK)
//#define PROFILE_END_TASK(TASK) Profiler::GetSingletonPtr()->EndTask(TASK)
}

#endif //AR_PROFILER_H
