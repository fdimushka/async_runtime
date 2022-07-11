#include "ar/ar.hpp"

using namespace AsyncRuntime;


void async_tcp(CoroutineHandler* handler, Yield<std::string>& yield, const std::string& host, int port, const std::string& req)
{
    auto connection = MakeTCPConnection(host.c_str(), port);

    yield( {} );

    int ret = Await(AsyncConnect(connection), handler);
    if(ret == IO_SUCCESS) {
        auto req_stream = MakeStream(req.c_str(), req.size());
        auto res_stream = MakeStream();
        ret = Await(AsyncWrite(connection, req_stream), handler);
        if(ret != IO_SUCCESS){
            std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
        }

        ret = Await(AsyncRead(connection, res_stream), handler);
        if(ret != IO_SUCCESS){
            std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
            yield( {} );
        }else{
            Await(AsyncClose(connection), handler);
            yield( { res_stream->GetBuffer() } );
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
                      "User-Agent: ar example tcp server\r\n"
                      "Accept: */*\r\n\r\n";

    std::string host = "93.184.216.34";
    auto coro = MakeCoroutine<std::string>(&async_tcp, host, 80, req);
    std::string result = Await(Async(coro));
    std::cout << result << std::endl;
    Terminate();
    return 0;
}