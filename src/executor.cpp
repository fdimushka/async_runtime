#include "ar/executor.hpp"


using namespace AsyncRuntime;


Executor::Executor(const std::string & name_, int max_processors_count_) :
            name(name_),
            notify_inc(0),
            max_processors_count(max_processors_count_)
{
    Spawn();
}


Executor::~Executor()
{
    for(auto & processor : processors)
    {
        processor->Terminate();
        delete processor;
        processor = nullptr;
    }
    processors.clear();
}


void Executor::Spawn()
{
    for(int i = 0; i < max_processors_count; ++i) {
        auto *processor = new Processor(this);
        processors.emplace_back(processor);
    }


    for(auto *processor : processors){
        processor->Run();
    }
}


void Executor::Post(Task *task)
{
    const auto& execute_state = task->GetDesirableExecutor();
    if(execute_state.processor != nullptr) {
        Processor * processor = execute_state.processor;
        processor->Post(task);
    }else{
        RunQueuePush(task);

        bool notified = false;
        for(size_t i = 0; i < processors.size(); ++i) {
            notify_inc = (notify_inc + 1) % processors.size();
            size_t pid = notify_inc;

            auto p_state = processors[pid]->GetState();
            if(p_state != Processor::EXECUTE) {
                notified = true;
                processors[pid]->Notify();
                break;
            }
        }

        if(!notified) {
            processors[notify_inc]->Notify();
        }
    }
}


void Executor::RunQueuePush(Task *task)
{
    std::lock_guard<std::mutex> lock(run_queue_mutex);
    run_queue.push(task);
}