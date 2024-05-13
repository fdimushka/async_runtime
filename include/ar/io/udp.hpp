#ifndef AR_UDP_HPP
#define AR_UDP_HPP

#define BOOST_THREAD_PROVIDES_FUTURE 1
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION 1

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/use_future.hpp>
#include "ar/task.hpp"

namespace AsyncRuntime::IO {
    using boost::asio::deadline_timer;
    using boost::asio::ip::udp;
    typedef boost::system::error_code error_code;
    typedef std::tuple<boost::system::error_code, std::size_t> read_result;

    class udp_session : public std::enable_shared_from_this<udp_session> {
    public:
        explicit udp_session(boost::asio::io_service& io_service)
        : socket(io_service, udp::endpoint(udp::v4(), 0))
        , resolver(io_service)
        , deadline(io_service)
        , read_timeout(0)
        {
        };

        ~udp_session() = default;

        void set_read_timeout(int seconds);

        int get_fd();

        size_t read_input_stream(char *buffer, size_t size);

        std::shared_ptr<udp_session> get_ptr() { return shared_from_this(); }

        future_t<error_code> async_connect(const char *ip_address, int port);

        future_t<read_result> async_read();

        future_t<error_code> async_write(const char* buffer, size_t size);

        void close();
    private:
        udp::socket socket;
        udp::endpoint sender_endpoint;
        udp::resolver resolver;
        deadline_timer deadline;
        boost::asio::streambuf input_buffer;
        int read_timeout;
    };

    typedef std::shared_ptr<udp_session> udp_session_ptr;
}

#endif //AR_TCP_HPP
