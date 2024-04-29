#ifndef AR_IO_H
#define AR_IO_H

#include "ar/task.hpp"
#include "ar/io/tcp.hpp"
#include "ar/io/udp.hpp"
#include "ar/io/http.hpp"

namespace AsyncRuntime::IO {
    typedef boost::system::error_code error_code;
    typedef std::tuple<boost::system::error_code, std::size_t> read_result;

    tcp_session_ptr MakeTCPSession();

    udp_session_ptr MakeUDPSession();

    http_session_ptr MakeHTTPSession();
}

#endif //AR_IO_H
