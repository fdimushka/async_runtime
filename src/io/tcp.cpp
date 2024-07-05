#include "ar/io/tcp.hpp"
#include "io_executor.h"
#include "io_task.h"
#include "ar/runtime.hpp"
#include "ar/logger.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::IO;

tcp_session::~tcp_session() {
    deadline.cancel();
}

void tcp_session::close() {
    auto executor = static_cast<IOExecutor*>(AsyncRuntime::Runtime::g_runtime->GetIOExecutor());
    auto self(shared_from_this());
    executor->Post([self](){
        try {
            self->socket.close();
        } catch (...) {
            AR_LOG_SS(Error, "socket close failed.")
        }
    });

    deadline.cancel();
}

void tcp_session::set_read_timeout(int seconds) {
    read_timeout = seconds;
    deadline.expires_from_now(boost::posix_time::seconds(seconds));
}

int tcp_session::get_fd() {
    return socket.lowest_layer().native_handle();
}

size_t tcp_session::read_input_stream(char *buffer, size_t size) {
    size_t min_size = std::min(input_buffer.size(), size);
    if (min_size > 0) {
        std::istream in(&input_buffer);
        in.read(buffer, min_size);
    }
    return min_size;
}

future_t<error_code> tcp_session::async_connect(const char *ip_address, int port) {
    error_code ec;
    auto task = std::make_shared<IO::io_task>();
    auto executor = static_cast<IOExecutor *>(Runtime::g_runtime->GetIOExecutor());
    socket.async_connect(executor->Resolve(ip_address, port), boost::bind(&IO::io_task::handler, task->get_ptr(), boost::placeholders::_1));
    return task->get_future();
}

future_t<read_result> tcp_session::async_read(size_t size) {
    if (read_timeout > 0) {
        auto executor = static_cast<IOExecutor*>(AsyncRuntime::Runtime::g_runtime->GetIOExecutor());
        auto self(shared_from_this());
        auto task = std::make_shared<IO::read_task>(&deadline);
        auto future = task->get_future();
        executor->Post([self, task, size]() {
            try {
                boost::asio::async_read(self->socket, self->input_buffer,
                                        boost::asio::transfer_at_least(size),
                                        boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1,
                                                    boost::placeholders::_2));
            } catch (...) {
                AR_LOG_SS(Error, "read to socket failed.")
                task->cancel();
            }
        });
        deadline.expires_from_now(boost::posix_time::seconds(read_timeout));
        deadline.async_wait(boost::bind(&IO::read_task::handler_deadline, task->get_ptr()));

        return std::move(future);
    } else {
        auto executor = static_cast<IOExecutor*>(AsyncRuntime::Runtime::g_runtime->GetIOExecutor());
        auto self(shared_from_this());
        auto task = std::make_shared<IO::read_task>();
        auto future = task->get_future();

        executor->Post([self, task, size]() {
            try {
                boost::asio::async_read(self->socket, self->input_buffer,
                                        boost::asio::transfer_at_least(size),
                                        boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1,
                                                    boost::placeholders::_2));
            } catch (...) {
                AR_LOG_SS(Error, "read to socket failed.")
                task->cancel();
            }
        });
        return std::move(future);
    }
}

future_t<read_result> tcp_session::async_read() {
    auto executor = static_cast<IOExecutor*>(AsyncRuntime::Runtime::g_runtime->GetIOExecutor());
    auto self(shared_from_this());

    if (read_timeout > 0) {
        auto task = std::make_shared<IO::read_task>(&deadline);
        auto future = task->get_future();
        executor->Post([self, task]() {
            try {
                boost::asio::async_read(self->socket, self->input_buffer,
                                        boost::asio::transfer_at_least(1),
                                        boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1,
                                                    boost::placeholders::_2));
            } catch (...) {
                AR_LOG_SS(Error, "read to socket failed.")
                task->cancel();
            }
        });

        deadline.expires_from_now(boost::posix_time::seconds(read_timeout));
        deadline.async_wait(boost::bind(&IO::read_task::handler_deadline, task->get_ptr()));

        return std::move(future);
    } else {
        auto task = std::make_shared<IO::read_task>();
        auto future = task->get_future();
        executor->Post([self, task]() {
            try {
                boost::asio::async_read(self->socket, self->input_buffer,
                                        boost::asio::transfer_at_least(1),
                                        boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1,
                                                    boost::placeholders::_2));
            } catch (...) {
                AR_LOG_SS(Error, "read to socket failed.")
                task->cancel();
            }
        });

        return std::move(future);
    }
}

future_t<error_code> tcp_session::async_write(const char *buffer, size_t size) {
    auto executor = static_cast<IOExecutor*>(AsyncRuntime::Runtime::g_runtime->GetIOExecutor());
    auto self(shared_from_this());
    auto task = std::make_shared<IO::io_task>();
    auto future = task->get_future();
    executor->Post([self, task, buffer, size]() {
        try {
            boost::asio::async_write(self->socket,
                                     boost::asio::buffer(buffer, size),
                                     boost::bind(&IO::io_task::handler, task->get_ptr(), boost::placeholders::_1));
        } catch (...) {
            AR_LOG_SS(Error, "write from socket failed.")
            task->cancel();
        }
    });

    return std::move(future);
}