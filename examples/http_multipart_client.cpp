#include "ar/ar.hpp"
#include "ar/io/io.hpp"
#include "ar/io/http_digest_authenticator.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;
using namespace boost::beast;

std::string build_body_req() {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<SubscribeEvent>\n"
           "    <heartbeat>30</heartbeat>\n"
           "    <eventMode>all</eventMode>\n"
           "</SubscribeEvent>";
}

void async_post(coroutine_handler* handler, yield<void> & yield) {
    std::string username = "admin";
    std::string password = "admin";
    std::string token;

    auto session = IO::MakeHTTPMultipartSession(100);
    IO::http_request req;
    req.method = IO::post;
    req.host = "0.0.0.0";
    req.port = 80;
    req.target = "/ISAPI/Event/notification/subscribeEvent";
    req.body = build_body_req();

    session->set_request_field(http::field::accept, "*/*");
    session->set_request_field(http::field::connection, "close");
    if (Await(session->async_request(req), handler)) {
        session->close();
        return;
    }

    if (session->get_response().result() == http::status::unauthorized ) {
        IO::http_digest_authenticator digest(session->get_response()[http::field::www_authenticate], username, password,
                                             req.target,
                                             to_string(session->get_request().method()));
        if(digest.generate_authorization()) {
            session->set_request_field(http::field::authorization, digest.get_authorization());
        }
    }

    session->close();

    auto ec = Await(session->async_request_multipart(req), handler);
    if (!ec) {
        if (session->get_response().result() == http::status::ok ) {
            for(;;) {
                ec = Await(session->async_read_part(), handler);
                if (ec) {
                    std::cout << ec.message() << std::endl;
                    break;
                } else {
                    const auto &res = session->get_response();
                    if (res[http::field::content_type] == "image/jpeg") {
                        std::cout << res[http::field::content_type] << std::endl;
                    } else {
                        std::cout << res[http::field::content_type] << std::endl;
                        std::cout << res.body() << std::endl;
                    }
                }
            }
        } else {
            std::cout << session->get_response().result() << std::endl;
        }
    } else {
        std::cout << ec.message() << std::endl;
    }

    session->close();
}

int main() {
    AsyncRuntime::Logger::s_logger.SetStd();
    SetupRuntime();

    Await(Async(make_coroutine(&async_post)));

    Terminate();
    return 0;
}