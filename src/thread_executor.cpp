#include "ar/thread_executor.hpp"

using namespace AsyncRuntime;


ThreadExecutor::~ThreadExecutor()
{
    Join();
}


void ThreadExecutor::Join()
{
    for(auto& thread : threads)
    {
        if(thread.joinable())
            thread.join();
    }

    threads.clear();
}


std::vector<std::thread::id> ThreadExecutor::GetThreadIds() const
{
    std::vector<std::thread::id> ids;

    for(const auto &th : threads)
        ids.push_back(th.get_id());

    return std::move(ids);
}

