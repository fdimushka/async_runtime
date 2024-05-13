#ifndef AR_HTTP_H
#define AR_HTTP_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
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
    class http_multipart_request_task;

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
        std::string body;
        std::string target;
    };

    class http_session : public std::enable_shared_from_this<http_session> {
        friend class http_request_task;
        friend class http_multipart_request_task;
    public:
        explicit http_session(boost::asio::io_service& io_service, int timeout = 0)
                : resolver(net::make_strand(io_service))
                , stream(net::make_strand(io_service))
                , timeout(timeout)
        {
        };

        void set_request_field(http::field field, const std::string & value);

        future_t<error_code> async_request(const http_request & request);

        void close();

        const http::request<http::string_body> & get_request() const { return req; }
        const http::response<http::string_body> & get_response() const { return res; }
    protected:
        int timeout;
        tcp::resolver resolver;
        beast::tcp_stream stream;
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::response<http::string_body> res;
    };

    class http_multipart_session : public http_session {
        friend class http_multipart_request_task;
        friend class http_multipart_part_request_task;
    public:
        explicit http_multipart_session(boost::asio::io_service& io_service, int timeout = 0)
                : http_session(io_service, timeout)
        {
        };

        future_t<error_code> async_request_multipart(const http_request & request);

        future_t<error_code> async_read_part();
    protected:
        boost::asio::streambuf input_buffer;
        std::string mime_boundary;
    };

    typedef std::shared_ptr<http_session> http_session_ptr;
    typedef std::shared_ptr<http_multipart_session> http_multipart_session_ptr;
}

#endif //AR_HTTP_H
