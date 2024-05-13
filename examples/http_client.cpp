#include "ar/ar.hpp"
#include "ar/io/io.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;

void async_client(coroutine_handler* handler, yield<void> & yield) {
    auto session = IO::MakeHTTPSession();
    IO::http_request req;
    req.method = IO::get;
    req.host = "www.example.com";
    req.port = 80;
    req.target = "/";

    auto ec = Await(session->async_request(req), handler);
    if (ec) {
        std::cerr << ec << std::endl;
    } else {
        std::cout << session->get_response() << std::endl;
    }

    session->close();
}

int main() {
    AsyncRuntime::Logger::s_logger.SetStd();
    SetupRuntime();

    Await(Async(make_coroutine(&async_client)));

    Terminate();
    return 0;
}