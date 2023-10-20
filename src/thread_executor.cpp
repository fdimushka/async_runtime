#include "ar/thread_executor.hpp"
#include "ar/logger.hpp"

using namespace AsyncRuntime;


void ThreadHelper::SetName(const char* name)
{
    int err = pthread_setname_np(pthread_self(), name);
    if(err)
        AR_LOG_SS(Error, "Error setting name for thread: " << err );
}


std::string ThreadHelper::GetName()
{
    size_t buff_size = 16;
    char buff[buff_size];
    int err = pthread_getname_np(pthread_self(), &buff[0], buff_size);
    if(err)
        AR_LOG_SS(Error, "Error getting name for thread: " << err );

    std::string str(&buff[0], buff_size);
    return std::move(str.substr (0,str.find('\0')));
}


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
