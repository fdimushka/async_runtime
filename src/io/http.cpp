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

future_t<error_code> http_session::async_request(const http_request & request) {
    //req_.version(version);
    req.method(get_method(request));
    req.set(http::field::host, request.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.target(request.target);

    auto task = std::make_shared<IO::http_request_task>(shared_from_this());
    resolver.async_resolve(
            request.host,
            std::to_string(request.port),
            beast::bind_front_handler(
                    &IO::http_request_task::handler_resolve,
                    task->get_ptr()));
    return task->get_future();
}

void http_session::close() {
    error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
}