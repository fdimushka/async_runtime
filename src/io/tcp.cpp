#include "ar/io/tcp.hpp"
#include "io_executor.h"
#include "io_task.h"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::IO;

void tcp_session::close() {
    auto executor = (IOExecutor*)Runtime::g_runtime->GetIOExecutor();
    socket.close();
    deadline.cancel();
//    executor->IOPost([this]() {
//        socket.close();
//        deadline.cancel();
//    });
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
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(ip_address, ec), port);

    if (ec) {
        return make_resolved_future(ec);
    }
    socket.async_connect(endpoint, boost::bind(&IO::io_task::handler, task->get_ptr(), boost::placeholders::_1));
    return task->get_future();
}

future_t<read_result> tcp_session::async_read() {
    if (read_timeout > 0) {
        auto task = std::make_shared<IO::read_task>(&deadline);
        //auto executor = (IOExecutor*)Runtime::g_runtime->GetIOExecutor();
//        executor->IOPost([this, task](){
//            boost::asio::async_read(socket, input_buffer,
//                                    boost::asio::transfer_at_least(1),
//                                    boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1, boost::placeholders::_2));
//
//            deadline.expires_from_now(boost::posix_time::seconds(read_timeout));
//            deadline.async_wait(boost::bind(&IO::read_task::handler_deadline, task->get_ptr()));
//        });
        boost::asio::async_read(socket, input_buffer,
                                boost::asio::transfer_at_least(1),
                                boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1, boost::placeholders::_2));

        deadline.expires_from_now(boost::posix_time::seconds(read_timeout));
        deadline.async_wait(boost::bind(&IO::read_task::handler_deadline, task->get_ptr()));

        return task->get_future();
    } else {
        auto task = std::make_shared<IO::read_task>();
        boost::asio::async_read(socket, input_buffer,
                                boost::asio::transfer_at_least(1),
                                boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1,
                                            boost::placeholders::_2));
//        auto executor = (IOExecutor*)Runtime::g_runtime->GetIOExecutor();
//            executor->IOPost([this, task]() {
//                boost::asio::async_read(socket, input_buffer,
//                                        boost::asio::transfer_at_least(1),
//                                        boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1,
//                                                    boost::placeholders::_2));
//        });
        return task->get_future();
    }
}

future_t<error_code> tcp_session::async_write(const char *buffer, size_t size) {
    auto task = std::make_shared<IO::io_task>();
    boost::asio::async_write(socket,
                             boost::asio::buffer(buffer, size),
                             boost::bind(&IO::io_task::handler, task->get_ptr(), boost::placeholders::_1));
    return task->get_future();
}