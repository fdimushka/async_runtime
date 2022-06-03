#include "ar/ar.hpp"
#include <cstring>


using namespace AsyncRuntime;


int main() {
    SetupRuntime();

    auto recv_udp = MakeUDP("0.0.0.0", 5000);
    auto send_udp = MakeUDP("0.0.0.0", 0);
    auto send_stream = MakeStream("Hello world!", 13);
    auto out_stream = MakeStream();

    int ret = Await(AsyncUDPBind(recv_udp, UV_UDP_REUSEADDR));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        goto end;
    }

    ret = Await(AsyncUDPBind(send_udp, 0, true));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        goto end;
    }

    ret = Await(AsyncSend(send_udp, send_stream, IPv4Addr{"0.0.0.0", 5000}));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        goto end;
    }

    ret = Await(AsyncRecv(recv_udp, out_stream));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        goto end;
    }
end:
    Terminate();
    return 0;
}