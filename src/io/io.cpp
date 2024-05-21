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

http_session_ptr AsyncRuntime::IO::MakeHTTPSession(int timeout) {
    auto executor = static_cast<IOExecutor*>(Runtime::g_runtime->GetIOExecutor());
    return executor->MakeHTTPSession(timeout);
}

http_multipart_session_ptr AsyncRuntime::IO::MakeHTTPMultipartSession(int timeout) {
    auto executor = static_cast<IOExecutor *>(Runtime::g_runtime->GetIOExecutor());
    return executor->MakeHTTPMultipartSession(timeout);
}

tcp_endpoint AsyncRuntime::IO::Resolve(const char *host, int port) {
    auto executor = static_cast<IOExecutor *>(Runtime::g_runtime->GetIOExecutor());
    return executor->Resolve(host, port);
}
