#include "ar/io/io.hpp"
#include "ar/runtime.hpp"
#include "io_executor.h"

using namespace AsyncRuntime;
using namespace AsyncRuntime::IO;

tcp_session_ptr AsyncRuntime::IO::MakeTCPSession() {
    auto executor = static_cast<IOExecutor*>(Runtime::g_runtime->GetIOExecutor());
    return executor->MakeTCPSession();
}

udp_session_ptr AsyncRuntime::IO::MakeUDPSession() {
    auto executor = static_cast<IOExecutor*>(Runtime::g_runtime->GetIOExecutor());
    return executor->MakeUDPSession();
}

http_session_ptr AsyncRuntime::IO::MakeHTTPSession() {
    auto executor = static_cast<IOExecutor*>(Runtime::g_runtime->GetIOExecutor());
    return executor->MakeHTTPSession();
}