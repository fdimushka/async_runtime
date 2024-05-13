#include "ar/io/http.hpp"
#include "io_http_task.h"

using namespace AsyncRuntime;
using namespace AsyncRuntime::IO;

http::verb get_method(const http_request & request) {
    switch (request.method) {
        case get : return http::verb::get;
        case post : return http::verb::post;
        case put : return http::verb::put;
        case del : return http::verb::delete_;
        case options : return http::verb::options;
        default:
            return http::verb::unknown;
    }
}

void http_session::set_request_field(http::field field, const std::string & value) {
    req.set(field, value);
}

future_t<error_code> http_session::async_request(const http_request & request) {
    req.method(get_method(request));
    req.set(http::field::host, request.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.target(request.target);

    if (request.method == post) {
        req.body() = request.body;
        req.prepare_payload();
    }

    res = {};
    buffer.clear();

    auto task = std::make_shared<IO::http_request_task>(shared_from_this());
    resolver.async_resolve(
            request.host,
            std::to_string(request.port),
            beast::bind_front_handler(
                    &IO::http_request_task::handler_resolve,
                    task->get_ptr()));
    return task->get_future();
}

future_t<error_code> http_multipart_session::async_request_multipart(const http_request & request) {
    req.method(get_method(request));
    req.set(http::field::host, request.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.target(request.target);

    if (request.method == post) {
        req.body() = request.body;
        req.prepare_payload();
    }

    res = {};
    buffer.clear();

    auto task = std::make_shared<IO::http_multipart_request_task>(shared_from_this());
    resolver.async_resolve(
            request.host,
            std::to_string(request.port),
            beast::bind_front_handler(
                    &IO::http_multipart_request_task::handler_resolve,
                    task->get_ptr()));
    return task->get_future();
}

future_t<error_code> http_multipart_session::async_read_part() {
    if (mime_boundary.empty()) {
        return make_resolved_future<error_code>(beast::make_error_code(static_cast<beast::error>(beast::errc::protocol_error)));
    }

    if (timeout > 0) {
        stream.expires_after(std::chrono::seconds(timeout));
    } else {
        stream.expires_after(std::chrono::seconds(60));
    }

    buffer.clear();
    auto task = std::make_shared<IO::http_multipart_part_request_task>(shared_from_this());
    boost::asio::streambuf::mutable_buffers_type mutable_buffer = input_buffer.prepare(1024);
    stream.async_read_some(mutable_buffer, beast::bind_front_handler(&http_multipart_part_request_task::handle_read, task->get_ptr()));
    return task->get_future();
}

void http_session::close() {
    error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
}