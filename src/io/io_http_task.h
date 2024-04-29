#ifndef AR_IO_HTTP_TASK_H
#define AR_IO_HTTP_TASK_H

#define BOOST_THREAD_PROVIDES_FUTURE 1
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION 1

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "ar/task.hpp"
#include "ar/io/http.hpp"

namespace AsyncRuntime::IO {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = boost::asio::ip::tcp;
    typedef beast::error_code error_code;

    class http_request_task : public std::enable_shared_from_this<http_request_task> {
        typedef promise_t<error_code> promise_type;
    public:
        explicit http_request_task(const http_session_ptr & session)
        : session(session)
        {
        };

        ~http_request_task() = default;

        void handler_resolve(error_code ec, tcp::resolver::results_type results) {
            if(ec) {
                promise.set_value(ec);
                return;
            }

            if (session->timeout > 0) {
                session->stream.expires_after(std::chrono::seconds(session->timeout));
            } else {
                session->stream.expires_after(std::chrono::seconds(30));
            }

            session->stream.async_connect(
                    results,
                    beast::bind_front_handler(
                            &http_request_task::handle_connect,
                            shared_from_this()));
        }

        void handle_connect(error_code ec, tcp::resolver::results_type::endpoint_type) {
            if(ec) {
                promise.set_value(ec);
                return;
            }

            if (session->timeout > 0) {
                session->stream.expires_after(std::chrono::seconds(session->timeout));
            } else {
                session->stream.expires_after(std::chrono::seconds(30));
            }

            http::async_write(session->stream, session->req,
                              beast::bind_front_handler(
                                      &http_request_task::handle_write,
                                      shared_from_this()));
        }

        void handle_write(error_code ec, std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);
            if(ec) {
                promise.set_value(ec);
                return;
            }

            http::async_read(session->stream, session->buffer, session->res,
                             beast::bind_front_handler(
                                     &http_request_task::handle_read,
                                     shared_from_this()));
        }

        void handle_read(beast::error_code ec, std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);
            promise.set_value(ec);
        }

        std::shared_ptr<http_request_task> get_ptr() { return shared_from_this(); };

        future_t<error_code> get_future() { return promise.get_future(); }
    private:
        http_session_ptr session;
        promise_type promise;
    };
}

#endif //AR_IO_HTTP_TASK_H
