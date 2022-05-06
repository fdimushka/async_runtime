#include "ar/tcp.hpp"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;


TCPConnectionPtr AsyncRuntime::MakeTCPConnection(const char *hostname, int port, int keepalive)
{
    TCPConnectionPtr connection = std::make_shared<TCPConnection>();
    connection->hostname = std::string{hostname};
    connection->port = port;
    connection->keepalive = keepalive;
    return connection;
}


TCPServerPtr AsyncRuntime::MakeTCPServer(const char* hostname, int port)
{
    TCPServerPtr server = std::make_shared<TCPServer>();
    server->hostname = std::string{hostname};
    server->port = port;
    return server;
}


TCPSession::TCPSession(uv_stream_t *server, const HandlerType & connection_handler) :
        fn(connection_handler),
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


void TCPSession::Invoke(CoroutineHandler *handler)
{
    if(fn)
        fn(handler, shared_from_this());
}


void TCPSession::Session(CoroutineHandler *handler, YieldVoid yield, const std::shared_ptr<TCPSession> &session) {
        //accept
        yield();

        session->Invoke(handler);
}
