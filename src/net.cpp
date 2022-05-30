#include "ar/net.hpp"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;


TCPConnectionPtr AsyncRuntime::MakeTCPConnection(const char *hostname, int port, int keepalive)
{
    TCPConnectionPtr connection = std::make_shared<TCPConnection>();
    connection->fd = -1;
    connection->hostname = std::string{hostname};
    connection->port = port;
    connection->read_task = nullptr;
    connection->keepalive = keepalive;
    connection->is_reading = false;
    connection->read_stream.SetMode(IOStream::Mode::R);
    return connection;
}


TCPConnectionPtr AsyncRuntime::MakeTCPConnection(int fd)
{
    TCPConnectionPtr connection = std::make_shared<TCPConnection>();
    connection->fd = fd;
    connection->read_task = nullptr;
    connection->is_reading = false;
    connection->read_stream.SetMode(IOStream::Mode::R);
    return connection;
}


TCPServerPtr AsyncRuntime::MakeTCPServer(const char* hostname, int port)
{
    TCPServerPtr server = std::make_shared<TCPServer>();
    server->hostname = std::string{hostname};
    server->port = port;
    return server;
}


UDPPtr AsyncRuntime::MakeUDP(const char* hostname, int port)
{
    UDPPtr udp = std::make_shared<UDP>();
    udp->hostname = std::string{hostname};
    udp->port = port;
    return udp;
}


NetAddrInfoPtr AsyncRuntime::MakeNetAddrInfo(const char* node)
{
    NetAddrInfoPtr info = std::make_shared<NetAddrInfo>();
    info->node = std::string{node};
    info->hints.ai_family = PF_INET;
    info->hints.ai_socktype = SOCK_STREAM;
    info->hints.ai_protocol = IPPROTO_TCP;
    info->hints.ai_flags = 0;
    return info;
}


TCPSession::TCPSession(uv_stream_t *server, const CallbackType & callback) :
        fn(callback),
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
        int fd;
        int error = uv_accept(server_, (uv_stream_t *) client_);
        RNT_ASSERT_MSG(error == 0, FSErrorMsg(error));

        uv_fileno((uv_handle_t *)client_, &fd);
        connection_ = MakeTCPConnection(fd);
        accepted_ = true;
    }
}


void TCPSession::Run() {
    coroutine = std::make_shared<Coroutine<void>>(&TCPSession::Session, shared_from_this());
    Async(*coroutine);
}


void TCPSession::Invoke(CoroutineHandler *handler)
{
    assert(connection_);

    int ret = Await(AsyncConnect(connection_), handler);
    if(ret == IO_SUCCESS && fn) {
        fn(handler, connection_);
    }

    Await(AsyncClose(connection_), handler);
}


void TCPSession::Session(CoroutineHandler *handler, YieldVoid yield, std::shared_ptr<TCPSession> session) {
        //accept
        yield();
        session->Invoke(handler);
}