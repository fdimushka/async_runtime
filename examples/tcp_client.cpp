#include "ar/ar.hpp"
#include "ar/io/io.hpp"
#include "ar/logger.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;


void async_client(coroutine_handler* handler, yield<void> & yield) {
    auto session = IO::MakeTCPSession();
    session->set_read_timeout(10);

    auto ec = Await(session->async_connect("127.0.0.1", 8080), handler);
    std::cout << "connected: " << ec << std::endl;

    for (;;) {
        auto res = Await(session->async_read(), handler);

        if (!std::get<IO::error_code>(res)) {
            std::size_t size = std::get<std::size_t>(res);

            if (size > 0)
            {
                char *buffer = (char*)std::malloc(size);
                size = session->read_input_stream(buffer, size);
                std::cout << "Received: " << buffer << "\n";

                ec = Await(session->async_write(buffer, size), handler);
                if (ec) {
                    break;
                }
                std::cout << "Sended: " << size << "\n";
            }
        } else {
            break;
        }
    }

    session->close();
    std::cout << "close" << std::endl;
}

int main() {
    AsyncRuntime::Logger::s_logger.SetStd();
    SetupRuntime();

    Await(Async(make_coroutine(&async_client)));

    Terminate();
    return 0;
}