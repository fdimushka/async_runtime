#ifndef AR_IO_EXECUTOR_H
#define AR_IO_EXECUTOR_H

#include "ar/executor.hpp"
#include "ar/io/tcp.hpp"
#include "ar/io/udp.hpp"
#include "ar/io/http.hpp"
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <oneapi/tbb.h>

namespace AsyncRuntime::IO {

    class IOExecutor : public IExecutor {
    public:
        explicit IOExecutor(const std::string & name_, int max_threads = 2);
        ~IOExecutor() noexcept override;

        IOExecutor(const IOExecutor&) = delete;
        IOExecutor(IOExecutor&&) = delete;
        IOExecutor& operator =(const IOExecutor&) = delete;
        IOExecutor& operator =(IOExecutor&&) = delete;

        tcp_session_ptr MakeTCPSession();

        udp_session_ptr MakeUDPSession();

        http_session_ptr MakeHTTPSession();

        void Post(const std::shared_ptr<task> & task) override;
    private:
        boost::asio::io_service io_service;
        boost::asio::io_service::work work;
        tbb::task_arena task_arenas;
        oneapi::tbb::task_group task_group;
    };
}

#endif //AR_IO_EXECUTOR_H
