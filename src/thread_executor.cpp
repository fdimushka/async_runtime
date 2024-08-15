#include "ar/thread_executor.hpp"
#include "ar/logger.hpp"

using namespace AsyncRuntime;


ThreadExecutor::~ThreadExecutor() {
    Join();
}


void ThreadExecutor::Join() {
    if(thread.joinable())
        thread.join();
}


int ThreadExecutor::SetAffinity(const CPU & affinity_cpu) {
    return AsyncRuntime::SetAffinity(thread, affinity_cpu);
}
