#include "io_executor.h"
#include "ar/logger.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::IO;

IOExecutor::IOExecutor(const std::string &name_, int max_threads)
    : IExecutor(name_, kIO_EXECUTOR)
    , work(io_service) {
    type = kIO_EXECUTOR;

    std::vector<tbb::numa_node_id> numa_indexes = tbb::info::numa_nodes();
    task_arenas.initialize(tbb::task_arena::constraints(numa_indexes[0]), 1, tbb::task_arena::priority::high);
    task_arenas.execute([this] {
        task_group.run(boost::bind(&boost::asio::io_service::run, &io_service));
    });
}

IOExecutor::~IOExecutor() noexcept {
    io_service.stop();
    task_arenas.execute([this] {
        task_group.wait();
    });
    task_arenas.terminate();
}

std::shared_ptr<tcp_session> IOExecutor::MakeTCPSession() {
    return std::make_shared<tcp_session>(io_service);
}

std::shared_ptr<udp_session> IOExecutor::MakeUDPSession() {
    return std::make_shared<udp_session>(io_service);
}

http_session_ptr IOExecutor::MakeHTTPSession() {
    return std::make_shared<http_session>(io_service);
}

void IOExecutor::Post(const std::shared_ptr<task> & task) {

}



