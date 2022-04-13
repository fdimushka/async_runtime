#include "ar/ar.hpp"


using namespace AsyncRuntime;


void handle_connection(TCPSessionPtr session) {
    std::cout << "new connection" << std::endl;
    auto in_stream = MakeStream();
    int ret = Await(AsyncRead(session, in_stream), session);
    if(ret == IO_SUCCESS) {
        std::cout << in_stream->GetBuffer() << std::endl;
        //open file
        //write
        //close file
    }

    Await(AsyncClose(session));
}


int main() {
    SetupRuntime();

    auto server = MakeTCPServer("0.0.0.0", 7000);
    Await(AsyncListen(server, &handle_connection));
    Terminate();
    return 0;
}
