#include "ar/ar.hpp"
using namespace AsyncRuntime;

void async_get_method(CoroutineHandler *handler, HTTPConnectionPtr http_connection)
{
    http_connection->AsyncResponse(HTTPStatus::OK, "Hello!!!");
}


int main() {
    SetupRuntime();

    HttpServer http_server;
    http_server.AddRoute("/", GET, &async_get_method);
    int ret = Await(http_server.AsyncBind("0.0.0.0", 7000));
    if(ret != IO_SUCCESS) {
        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
    }

    Terminate();
    return 0;
}