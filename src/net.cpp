#include "ar/net.hpp"

#include <istream>
#include <utility>
#include "ar/runtime.hpp"

using namespace AsyncRuntime;


TCPConnectionPtr AsyncRuntime::MakeTCPConnection(const char *hostname, int port, int keepalive)
{
    TCPConnectionPtr connection = std::make_shared<TCPConnection>();
    connection->fd = -1;
    connection->hostname = std::string{hostname};
    connection->port = port;
    connection->keepalive = keepalive;
    return connection;
}

TCPConnectionPtr AsyncRuntime::MakeTCPConnection(int fd)
{
    TCPConnectionPtr connection = std::make_shared<TCPConnection>();
    connection->fd = fd;
    return connection;
}

void AsyncRuntime::CloseReadStream(TCPConnection *conn, int error) {
    conn->read_error.store(error, std::memory_order_relaxed);
    if (conn->read_result && !conn->read_result->Resolved()) {
        conn->read_result->SetValue(error);
    }
}

void AsyncRuntime::ProduceReadStream(TCPConnection *conn, char* buffer, size_t size) {
    std::lock_guard<std::mutex> lock(conn->mutex);
    std::ostream out(&conn->read_stream);
    out.write(buffer, size);
    if (conn->read_result && !conn->read_result->Resolved()) {
        conn->read_result->SetValue(conn->read_stream.size());
    }
}

int AsyncRuntime::ConsumeReadStream(const std::shared_ptr<TCPConnection> & conn, char* buffer, size_t size) {
    return ConsumeReadStream(conn.get(), buffer, size);
}

int AsyncRuntime::ConsumeReadStream(TCPConnection *conn, char* buffer, size_t buf_size) {
    std::lock_guard<std::mutex> lock(conn->mutex);
    size_t size = std::min(conn->read_stream.size(), buf_size);
    if (size > 0) {
        std::istream in(&conn->read_stream);
        in.read(buffer, size);
    }
    return size;
}

std::shared_ptr<AsyncRuntime::Result<int>>
AsyncRuntime::AsyncWaitReadStream(const std::shared_ptr<TCPConnection> & conn, size_t buf_size) {
    std::lock_guard<std::mutex> lock(conn->mutex);
    if(conn->read_stream.size() <= 0) {
        conn->read_result = std::make_shared<AsyncRuntime::Result<int>>();
        return conn->read_result;
    } else {
        size_t size = std::min(conn->read_stream.size(), buf_size);
        return std::make_shared<AsyncRuntime::Result<int>>(size);
    }
}

TCPServerPtr AsyncRuntime::MakeTCPServer(const char* hostname, int port)
{
    TCPServerPtr server = std::make_shared<TCPServer>();
    server->hostname = std::string{hostname};
    server->port = port;
    return server;
}

TCPServerPtr
AsyncRuntime::MakeTCPServer(const char *hostname,
                            int port,
                            const std::function<void(void)> &on_bind_success,
                            const std::function<void(int)> &on_bind_error)
{
    TCPServerPtr server = std::make_shared<TCPServer>();
    server->hostname = std::string{hostname};
    server->port = port;
    server->on_bind_error = on_bind_error;
    server->on_bind_success = on_bind_success;
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


void AsyncRuntime::FlushUDPReceivedData(std::vector<UDPReceivedData> &all_recv_data)
{
    for(auto  &item : all_recv_data) {
        if(item.buf) {
            free(item.buf);
        }
    }

    all_recv_data.clear();
}


TCPSession::TCPSession(uv_stream_t *server, CallbackType  callback) :
        fn(std::move(callback)),
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
    if (!accepted_) {
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

