#include "ar/io/udp.hpp"
#include "io_task.h"

using namespace AsyncRuntime;
using namespace AsyncRuntime::IO;

void udp_session::close() {
    socket.close();
    deadline.cancel();
}

void udp_session::set_read_timeout(int seconds) {
    read_timeout = seconds;
    deadline.expires_from_now(boost::posix_time::seconds(seconds));
}

size_t udp_session::read_input_stream(char *buffer, size_t size) {
    size_t min_size = std::min(input_buffer.size(), size);
    if (min_size > 0) {
        std::istream in(&input_buffer);
        in.read(buffer, min_size);
    }
    return min_size;
}

int udp_session::get_fd() {
    return socket.lowest_layer().native_handle();
}

future_t<error_code> udp_session::async_connect(const char *ip_address, int port) {
    error_code ec;
    auto task = std::make_shared<IO::task>();
    udp::resolver::query query(udp::v4(), ip_address, std::to_string(port));
    udp::resolver::iterator itr = resolver.resolve(query);
    sender_endpoint = *itr;

//    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(ip_address, ec), port);
//
//    if (ec) {
//        return make_resolved_future(ec);
//    }
    //socket.async_connect(endpoint, boost::bind(&IO::task::handler, task->get_ptr(), _1));
    //return task->get_future();
    return make_resolved_future(ec);
}

future_t<read_result> udp_session::async_read() {
    boost::asio::streambuf::mutable_buffers_type mutable_buffer = input_buffer.prepare(1024);
    if (read_timeout > 0) {
        auto task = std::make_shared<IO::read_task>(&deadline);
        socket.async_receive_from(mutable_buffer, sender_endpoint,
                                  boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1, boost::placeholders::_2));
        deadline.expires_from_now(boost::posix_time::seconds(read_timeout));
        deadline.async_wait(boost::bind(&IO::read_task::handler_deadline, task->get_ptr()));
        return task->get_future();
    }else {
        auto task = std::make_shared<IO::read_task>();
        socket.async_receive_from(mutable_buffer, sender_endpoint,
                                  boost::bind(&IO::read_task::handler, task->get_ptr(), boost::placeholders::_1, boost::placeholders::_2));
        return task->get_future();
    }
}

future_t<error_code> udp_session::async_write(const char *buffer, size_t size) {
    auto task = std::make_shared<IO::task>();
    socket.async_send_to(boost::asio::buffer(buffer, size),
                         sender_endpoint,
                         boost::bind(&IO::task::handler, task->get_ptr(), boost::placeholders::_1));
    return task->get_future();
}