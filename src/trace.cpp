#include "trace.h"
#include "ar/helper.hpp"
#include "base64.h"

#ifdef USE_TRACE
#include "TraceSchema_generated.h"
#endif

#ifdef USE_TRACE

#include <regex>
#include <fstream>
#include <boost/algorithm/string/replace.hpp>

using namespace AsyncRuntime;

#define DEFAULT_FILE_NAME "trace"

#define TEMPLATE_PATH "data/templates/trace.html"
#define TEMPLATE_DATA_FIELD "{{ trace_data }}"
#define HTML_GENERATED_TRACE_FILE_NAME "trace.html"

AsyncRuntime::WorkTracer AsyncRuntime::WorkTracer::s_tracer;


WorkTracer::WorkTracer(int64_t store_interval_) :
store_interval(store_interval_)
, last_store_ts(NowTimeMS())
, start_ts(NowTimeNS())
{ }


void WorkTracer::AddWork(ObjectID processor_id, ObjectID actor_id, int64_t start_work_ts, int64_t end_work_ts)
{
    std::lock_guard<std::mutex> lock(mutex);
    works.push_back(Work{processor_id, actor_id, start_work_ts, end_work_ts});
    processors.insert(processor_id);

    if(NowTimeMS() - last_store_ts >= store_interval)
    {
        GenerateTraceSummery();
        last_store_ts = NowTimeMS();
    }
}


uint8_t* WorkTracer::PrepareStoreData(size_t& size)
{
    flatbuffers::FlatBufferBuilder  fbb(1024*1024);
    std::vector<AsyncRuntime::TraceWorkModel> fb_works;
    std::vector<uint32_t> processors_vec;

    for(const auto& work : works)
    {
        fb_works.emplace_back(AsyncRuntime::TraceWorkModel(work.processor_id, work.actor_id, work.start_work_ts, work.end_work_ts));
    }

    for(const auto& processor_id : processors)
    {
        processors_vec.push_back(processor_id);
    }

    auto fb_processors = fbb.CreateVector(processors_vec);
    auto fb_trace = CreateTraceModel(fbb,
                                     start_ts,
                                     NowTimeNS(),
                                     fb_processors,
                                     fbb.CreateVectorOfStructs<AsyncRuntime::TraceWorkModel>(&fb_works[0], fb_works.size()));
    fbb.Finish(fb_trace);

    auto* data = (uint8_t*)malloc(fbb.GetSize());
    size = fbb.GetSize();
    std::memcpy(data, fbb.GetBufferPointer(), fbb.GetSize());
    return data;
}


void WorkTracer::Store()
{
    size_t size = 0;
    uint8_t* data = PrepareStoreData(size);

    if(file_name.empty())
        file_name = DEFAULT_FILE_NAME;

    std::ofstream stream(file_name);
    stream.write((char*)data, size);
    stream.close();

    free(data);
}


void WorkTracer::GenerateTraceSummery()
{
    size_t size = 0;
    uint8_t* data = PrepareStoreData(size);

    std::ifstream in_stream(TEMPLATE_PATH);
    std::stringstream str_stream;
    str_stream << in_stream.rdbuf();
    std::string html_tmpl_str = str_stream.str();
    in_stream.close();

    std::string html_str =  boost::replace_all_copy(html_tmpl_str,
                                                    TEMPLATE_DATA_FIELD,
                                                    Base64Encode(std::string((char*)data, size)));

    std::ofstream out_stream(HTML_GENERATED_TRACE_FILE_NAME);
    out_stream << html_str;
    out_stream.close();

}


void WorkTracer::AsyncStore()
{
    Store();
}

#endif