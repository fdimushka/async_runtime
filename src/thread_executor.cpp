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

