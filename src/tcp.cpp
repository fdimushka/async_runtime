#include "ar/tcp.hpp"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;


TCPServerPtr AsyncRuntime::MakeTCPServer(const char* hostname, int port)
{
    TCPServerPtr server = std::make_shared<TCPServer>();
    server->hostname = std::string{hostname};
    server->port = port;
    return server;
}


TCPSession::TCPSession(uv_stream_t *server, const std::function<void(std::shared_ptr<TCPSession>)> & connection_handler) :
        handler(connection_handler),
        coroutine_handler_(nullptr),
        loop_(server->loop),
        server_(server),
        client_(nullptr),
        accepted_(false) {
    client_ = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop_, client_);
}


TCPSession::~TCPSession()
{
    if(client_ != nullptr)
        free(client_);
}


void TCPSession::Accept()
{
    if(!accepted_) {
        int error = uv_accept(server_, (uv_stream_t *) client_);
        RNT_ASSERT_MSG(error == 0, FSErrorMsg(error));
        accepted_ = true;
    }
}


void TCPSession::Run() {
    coroutine = std::make_shared<Coroutine<void>>(&TCPSession::Session, shared_from_this());
    Async(*coroutine);
}


void TCPSession::Invoke(CoroutineHandler *coroutine_handler) {
    coroutine_handler_ = coroutine_handler;
    if(handler)
        handler(shared_from_this());
}