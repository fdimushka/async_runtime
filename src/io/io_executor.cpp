#include "io_executor.h"
#include "ar/logger.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::IO;

IOExecutor::IOExecutor(const std::string &name_, int max_threads)
    : IExecutor(name_, kIO_EXECUTOR)
    , work(io_service) {
    type = kIO_EXECUTOR;

    for (int i = 0; i < max_threads; ++i) {
        thread_pool.emplace_back(std::thread(boost::bind(&boost::asio::io_service::run, &io_service)));
    }
}

IOExecutor::~IOExecutor() noexcept {
    io_service.stop();
    for (auto &th : thread_pool) {
        if (th.joinable())
            th.join();
    }
}

std::shared_ptr<tcp_session> IOExecutor::MakeTCPSession() {
    return std::make_shared<tcp_session>(io_service);
}

std::shared_ptr<udp_session> IOExecutor::MakeUDPSession() {
    return std::make_shared<udp_session>(io_service);
}

http_session_ptr IOExecutor::MakeHTTPSession(int timeout) {
    return std::make_shared<http_session>(io_service, timeout);
}

http_multipart_session_ptr IOExecutor::MakeHTTPMultipartSession(int timeout) {
    return std::make_shared<http_multipart_session>(io_service, timeout);
}

tcp_endpoint IOExecutor::Resolve(const char *host, int port) {
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), host, std::to_string(port));
    tcp::resolver::iterator iterator = resolver.resolve(query);
    return *iterator;
}

void IOExecutor::Post(task *task) {
    throw std::runtime_error("task not supported in IOExecutor use io_task");
}


void Post(const io_task_ptr & task) {
}

void Post(const read_task_ptr & task) {
}



