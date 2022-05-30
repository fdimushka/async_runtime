#include "ar/ar.hpp"


using namespace AsyncRuntime;


const char* make_discover_msg() {
    char* buffer = (char*)malloc(256);
    memset(buffer, 0, 256);

    // BOOTREQUEST
    buffer[0] = 0x1;
    // HTYPE ethernet
    buffer[1] = 0x1;
    // HLEN
    buffer[2] = 0x6;
    // HOPS
    buffer[3] = 0x0;
    // XID 4 bytes
    buffer[4] = (unsigned int) random();
    // SECS
    buffer[8] = 0x0;
    // FLAGS
    buffer[10] = 0x80;
    // CIADDR 12-15 is all zeros
    // YIADDR 16-19 is all zeros
    // SIADDR 20-23 is all zeros
    // GIADDR 24-27 is all zeros
    // CHADDR 28-43 is the MAC address, use your own
    buffer[28] = 0xe4;
    buffer[29] = 0xce;
    buffer[30] = 0x8f;
    buffer[31] = 0x13;
    buffer[32] = 0xf6;
    buffer[33] = 0xd4;
    // SNAME 64 bytes zero
    // FILE 128 bytes zero
    // OPTIONS
    // - magic cookie
    buffer[236] = 99;
    buffer[237] = 130;
    buffer[238] = 83;
    buffer[239] = 99;

    // DHCP Message type
    buffer[240] = 53;
    buffer[241] = 1;
    buffer[242] = 1; // DHCPDISCOVER

    // DHCP Parameter request list
    buffer[243] = 55;
    buffer[244] = 4;
    buffer[245] = 1;
    buffer[246] = 3;
    buffer[247] = 15;
    buffer[248] = 6;

    return buffer;
}


int main() {
    SetupRuntime();

    auto recv_udp = MakeUDP("0.0.0.0", 68);
    auto send_udp = MakeUDP("0.0.0.0", 0);
    const char *discover_msg = make_discover_msg();
    auto send_stream = MakeStream(discover_msg, 256);

    int ret = Await(AsyncUDPBind(recv_udp));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        goto end;
    }

    ret = Await(AsyncUDPBind(send_udp, true));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        goto end;
    }

    ret = Await(AsyncSend(send_udp, send_stream, IPv4Addr{"255.255.255.255", 67}));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        goto end;
    }

end:
    free((char*)discover_msg);
    Terminate();
    return 0;
}