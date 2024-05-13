#ifndef AR_IO_EXECUTOR_H
#define AR_IO_EXECUTOR_H

#include "ar/executor.hpp"
#include "ar/io/tcp.hpp"
#include "ar/io/udp.hpp"
#include "ar/io/http.hpp"
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include "io_task.h"

namespace AsyncRuntime::IO {

    class IOExecutor : public IExecutor {
    public:
        explicit IOExecutor(const std::string & name_, int max_threads = 4);
        ~IOExecutor() noexcept override;

        IOExecutor(const IOExecutor&) = delete;
        IOExecutor(IOExecutor&&) = delete;
        IOExecutor& operator =(const IOExecutor&) = delete;
        IOExecutor& operator =(IOExecutor&&) = delete;

        tcp_session_ptr MakeTCPSession();

        udp_session_ptr MakeUDPSession();

        http_session_ptr MakeHTTPSession(int timeout = 0);

        http_multipart_session_ptr MakeHTTPMultipartSession(int timeout = 0);

        void Post(task *task) override;

        void Post(const io_task_ptr & task);

        void Post(const read_task_ptr & task);
    private:
        boost::asio::io_service io_service;
        boost::asio::io_service::work work;
        std::vector<std::thread> thread_pool;
    };
}

#endif //AR_IO_EXECUTOR_H
