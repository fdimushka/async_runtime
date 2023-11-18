#include "ar/ar.hpp"


using namespace AsyncRuntime;


void handle_connection(CoroutineHandler *handler, const TCPConnectionPtr& connection) {
    std::cout << "new connection " << std::this_thread::get_id() << std::endl;
    std::vector<char> buffer(65536);
    int res_size = Await(AsyncRead(handler, connection, buffer.data(), buffer.size()), handler);
    if(res_size > 0) {
        std::string response = "HTTP/1.1 200 OK\n"
                               "Server: ar tcp server\n"
                               "Accept-Ranges: bytes\n"
                               "Content-Length: 12\n"
                               "Connection: close\n"
                               "Content-Type: text/html\n"
                               "\n"
                               "Hello world!";
        int ret = Await(AsyncWrite(connection, response.c_str(), response.size()), handler);
        if(ret != IO_SUCCESS){
            std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        }
    }else{
        std::cerr << "reading error" << std::endl;
    }
}


int main() {
    SetupRuntime();
    auto server = MakeTCPServer("0.0.0.0", 7000);
    int ret = Await(AsyncListen(server, &handle_connection));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
    }
    Terminate();
    return 0;
}
