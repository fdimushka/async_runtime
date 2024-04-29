#ifndef AR_HTTP_H
#define AR_HTTP_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "ar/task.hpp"

namespace AsyncRuntime::IO {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = boost::asio::ip::tcp;
    typedef beast::error_code error_code;

    class http_request_task;

    enum http_method {
        get = 0,
        post,
        put,
        del,
        options
    };

    struct http_request {
        http_method method;
        std::string host;
        int port;
        std::string target;
    };

    class http_session : public std::enable_shared_from_this<http_session> {
        friend class http_request_task;
    public:
        explicit http_session(boost::asio::io_service& io_service)
                : resolver(net::make_strand(io_service))
                , stream(net::make_strand(io_service))
                , timeout(0)
        {
        };

        future_t<error_code> async_request(const http_request & request);

        void close();

        http::response<http::string_body> res;
    private:
        int timeout;
        tcp::resolver resolver;
        beast::tcp_stream stream;
        beast::flat_buffer buffer;
        http::request<http::empty_body> req;
    };

    typedef std::shared_ptr<http_session> http_session_ptr;
}

#endif //AR_HTTP_H
