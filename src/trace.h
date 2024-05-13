//
// Created by df on 12/27/21.
//

#ifndef AR_TRACE_H
#define AR_TRACE_H

#include "ar/object.hpp"

#include <vector>
#include <set>

#ifdef USE_TRACE
namespace AsyncRuntime {


    class WorkTracer {
        struct Work {
            ObjectID        processor_id;
            ObjectID        actor_id;
            int64_t         start_work_ts;
            int64_t         end_work_ts;
        };


    public:
        static WorkTracer s_tracer;

        WorkTracer(int64_t store_interval_ = 5000);
        virtual ~WorkTracer() = default;

        WorkTracer(const WorkTracer&) = delete;
        WorkTracer& operator =(const WorkTracer&) = delete;
        WorkTracer(WorkTracer&&) = delete;
        WorkTracer& operator =(WorkTracer&&) = delete;


        /**
         * @brief
         * @param file_name_
         */
        void SetStoreFile(const std::string& file_name_) { file_name = file_name_; }


        /**
         * @brief
         * @param processor_id
         * @param actor_id
         * @param start_work
         * @param end_work
         */
        void AddWork(ObjectID processor_id, ObjectID actor_id, int64_t start_work_ts, int64_t end_work_ts);
    private:
        uint8_t* PrepareStoreData(size_t& size);
        void AsyncStore();
        void Store();
        void GenerateTraceSummery();


        std::string                             file_name;
        std::mutex                              mutex;
        std::vector<Work>                       works;
        std::set<ObjectID>                      processors;
        int64_t                                 store_interval;
        int64_t                                 last_store_ts;
        int64_t                                 start_ts;
    };


    inline void TraceWork(ObjectID processor_id, ObjectID actor_id, int64_t start_work_ts, int64_t end_work_ts)
    {
        AsyncRuntime::WorkTracer::s_tracer.AddWork(processor_id, actor_id, start_work_ts, end_work_ts);
    }

#define TRACE_WORK(processor_id, actor_id, start_work_ts, end_work_ts)              \
    {                                                                               \
        AsyncRuntime::TraceWork(processor_id, actor_id, start_work_ts, end_work_ts);   \
    }
}
#endif

#endif //AR_TRACE_H
