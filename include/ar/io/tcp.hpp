#ifndef AR_TCP_HPP
#define AR_TCP_HPP

#define BOOST_THREAD_PROVIDES_FUTURE 1
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION 1

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/use_future.hpp>
#include "ar/task.hpp"

namespace AsyncRuntime::IO {
    using boost::asio::deadline_timer;
    using boost::asio::ip::tcp;
    typedef boost::system::error_code error_code;
    typedef std::tuple<boost::system::error_code, std::size_t> read_result;
    typedef boost::asio::ip::tcp::endpoint tcp_endpoint;

    class tcp_session : public std::enable_shared_from_this<tcp_session> {
    public:
        explicit tcp_session(boost::asio::io_service& io_service)
        : socket(io_service)
        , deadline(io_service)
        , read_timeout(0)
        {
        };

        ~tcp_session() = default;

        void set_read_timeout(int seconds);

        std::shared_ptr<tcp_session> get_ptr() { return shared_from_this(); }

        int get_fd();

        size_t read_input_stream(char *buffer, size_t size);

        future_t<error_code> async_connect(const char *ip_address, int port);

        future_t<read_result> async_read();

        future_t<error_code> async_write(const char* buffer, size_t size);

        void close();
    private:
        tcp::socket socket;
        deadline_timer deadline;
        boost::asio::streambuf input_buffer;
        int read_timeout;
    };

    typedef std::shared_ptr<tcp_session> tcp_session_ptr;
}

#endif //AR_TCP_HPP
