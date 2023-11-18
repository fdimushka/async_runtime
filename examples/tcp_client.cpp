#include "ar/ar.hpp"

using namespace AsyncRuntime;


void async_tcp(CoroutineHandler* handler, Yield<std::string>& yield, const std::string& host, int port, const std::string& req)
{
    auto connection = MakeTCPConnection(host.c_str(), port);

    yield( {} );

    int ret = Await(AsyncConnect(connection), handler);
    if(ret == IO_SUCCESS) {
        ret = Await(AsyncWrite(connection, req.c_str(), req.size()), handler);
        if(ret != IO_SUCCESS){
            std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        }

        std::vector<char> buffer(65536);
        int res_size = Await(AsyncRead(handler, connection, buffer.data(), buffer.size()), handler);
        if(res_size > 0) {
            Await(AsyncClose(connection), handler);
            yield( { std::string(buffer.data(), res_size) } );
        }else{
            Await(AsyncClose(connection), handler);
            std::cerr << "read error"<< std::endl;
            yield( {} );
        }
    }else{
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        yield( {} );
    }
}


int main() {
    SetupRuntime();
    std::string req = "GET / HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "Connection: close\r\n"
                      "User-Agent: ar example tcp server\r\n"
                      "Accept: */*\r\n\r\n";

    {
        std::string host = "93.184.216.34";
        auto coro = MakeCoroutine<std::string>(&async_tcp, host, 80, req);
        std::string result = Await(Async(coro));
        std::cout << result << std::endl;
    }
    Terminate();
    return 0;
}
