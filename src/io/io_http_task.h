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
#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/streambuf.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <regex>

#include "ar/task.hpp"
#include "ar/io/http.hpp"

namespace AsyncRuntime::IO {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = boost::asio::ip::tcp;
    typedef beast::error_code error_code;
    using string_view = boost::core::string_view;

    const static std::string boundary_type = "boundary=";

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
                session->stream.expires_after(std::chrono::seconds(60));
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
                session->stream.expires_after(std::chrono::seconds(60));
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


    class http_multipart_request_task : public std::enable_shared_from_this<http_multipart_request_task> {
        typedef promise_t<error_code> promise_type;
    public:
        explicit http_multipart_request_task(const http_session_ptr & session)
                : session(std::static_pointer_cast<http_multipart_session>(session))
        {
            parser.eager(false);
        };

        explicit http_multipart_request_task(const http_multipart_session_ptr & session)
                : session(session)
        {
            parser.eager(false);
        };

        ~http_multipart_request_task() = default;

        void handler_resolve(error_code ec, tcp::resolver::results_type results) {
            if(ec) {
                promise.set_value(ec);
                return;
            }

            if (session->timeout > 0) {
                session->stream.expires_after(std::chrono::seconds(session->timeout));
            } else {
                session->stream.expires_after(std::chrono::seconds(60));
            }

            session->stream.async_connect(
                    results,
                    beast::bind_front_handler(
                            &http_multipart_request_task::handle_connect,
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
                session->stream.expires_after(std::chrono::seconds(60));
            }

            http::async_write(session->stream, session->req,
                              beast::bind_front_handler(
                                      &http_multipart_request_task::handle_write,
                                      shared_from_this()));
        }

        void handle_write(error_code ec, std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);
            if(ec) {
                promise.set_value(ec);
                return;
            }

            parser.eager(false);
            boost::asio::streambuf::mutable_buffers_type mutable_buffer = session->input_buffer.prepare(512);
            session->stream.async_read_some(mutable_buffer,
                                            beast::bind_front_handler(&http_multipart_request_task::handle_read_header, shared_from_this()));

        }

        void handle_read_header(const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                session->input_buffer.commit(bytes_transferred);
                std::istream header_part(&session->input_buffer);
                std::string line;

                while (std::getline(header_part, line)) {
                    auto key_value_separator = line.find(':');
                    auto key = line.substr(0, key_value_separator);
                    auto value = line.substr(key_value_separator + 2, line.size() - key.size() - 3);
                    if (key.find("Content-Type") != std::string::npos) {
                        session->res.set(http::field::content_type, value);
                        std::string content_type_part;
                        std::istringstream content_type(value);
                        while (std::getline(content_type, content_type_part, ';')) {
                            if (auto idx = content_type_part.find(boundary_type); idx != std::string::npos) {
                                session->mime_boundary = content_type_part.substr(idx+boundary_type.size());
                                break;
                            }
                        }

                        break;
                    }
                }

                promise.set_value(ec);
            } else {
                promise.set_value(ec);
            }
        }

        std::shared_ptr<http_multipart_request_task> get_ptr() { return shared_from_this(); };

        future_t<error_code> get_future() { return promise.get_future(); }
    private:
        http::response_parser<http::string_body> parser;
        http_multipart_session_ptr session;
        promise_type promise;
    };

    class http_multipart_part_request_task : public std::enable_shared_from_this<http_multipart_part_request_task> {
        typedef promise_t<error_code> promise_type;
    public:
        explicit http_multipart_part_request_task(const http_session_ptr & session)
                : session(std::static_pointer_cast<http_multipart_session>(session))
        {
            parser.eager(true);
            is_part_header_done = false;
        };

        explicit http_multipart_part_request_task(const http_multipart_session_ptr & session)
                : session(session)
        {
            parser.eager(true);
            is_part_header_done = false;
        };

        ~http_multipart_part_request_task() = default;

        std::string buffer_to_string(const boost::asio::streambuf &buffer)
        {
            using boost::asio::buffers_begin;

            auto bufs = buffer.data();
            std::string result(buffers_begin(bufs), buffers_begin(bufs) + buffer.size());
            return result;
        }

        void handle_read(const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                session->input_buffer.commit(bytes_transferred);
                boost::system::error_code parser_ec;
                if (!is_part_header_done) {
                    std::istream stream_part(&session->input_buffer);
                    std::string line;

                    while (std::getline(stream_part, line)) {
                        if (line.find("--" + session->mime_boundary) != std::string::npos) {
                            parser.put(boost::asio::buffer("HTTP/1.1 200 OK\r\n"), parser_ec);
                            is_part_header_done = true;


                            if (stream_part.peek() == '\r') {
                                std::getline(stream_part, line);
                            }

                            break;
                        }
                    }
                }

                if (is_part_header_done) {
                    int consumed = parser.put(session->input_buffer.data(), parser_ec);
                    session->input_buffer.consume(consumed);

                    if (parser_ec) {
                        promise.set_value(parser_ec);
                        return;
                    }

                    if (parser.is_done()) {
                        session->res = parser.get();
                        promise.set_value(ec);
                        return;
                    }
                }

                boost::asio::streambuf::mutable_buffers_type mutable_buffer = session->input_buffer.prepare(1024);
                session->stream.async_read_some(mutable_buffer,
                                                beast::bind_front_handler(&http_multipart_part_request_task::handle_read, shared_from_this()));
            } else {
                promise.set_value(ec);
            }
        }

        std::shared_ptr<http_multipart_part_request_task> get_ptr() { return shared_from_this(); };

        future_t<error_code> get_future() { return promise.get_future(); }
    private:
        bool is_part_header_done;
        http::response_parser<http::string_body> parser;
        http_multipart_session_ptr session;
        promise_type promise;
    };
}

#endif //AR_IO_HTTP_TASK_H
