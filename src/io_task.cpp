#include "ar/io_task.hpp"
#include "ar/logger.hpp"
#include "uv.h"

#include <cstring>


using namespace AsyncRuntime;
#define MAX_READING_SIZE    1024*1024*10


const char* AsyncRuntime::FSErrorMsg(int error)
{
    return uv_strerror(error);
}


const char* AsyncRuntime::FSErrorName(int error)
{
    return uv_err_name(error);
}


bool FsOpenTask::Execute(uv_loop_t *loop)
{
    _request.data = this;
    uv_fs_open(loop, &_request, _filename, _flags, _mode, &FsOpenCb);
    return true;
}


bool FsReadTask::Execute(uv_loop_t *loop)
{
    _request.data = this;
    _stream->SetMode(IOStream::R);
    uv_buf_t *buf = _stream->Next();
    int error = uv_fs_read(loop, &_request, _fd, buf, 1, _seek, &FsReadCb);
    if(error){
        Resolve(error);
        return false;
    }
    return true;
}

bool FsWriteTask::Execute(uv_loop_t *loop)
{
    _request.data = this;
    _stream->SetMode(IOStream::W);
    uv_buf_t *buf = _stream->Next();
    if(buf) {
        uv_fs_write(loop, &_request, _fd, buf, 1, _seek, &FsWriteCb);
        return true;
    }else{
        Resolve(EIO);
        return false;
    }
}

bool FsCloseTask::Execute(uv_loop_t *loop)
{
    _request.data = this;
    uv_fs_close(loop, &_request, _fd, &FsCloseCb);
    return true;
}


bool NetConnectionTask::Execute(uv_loop_t *loop)
{
    assert(_connection);
    uv_connect_t *con = &_connection->connect;
    con->data = this;
    uv_tcp_init(loop, &_connection->socket);

    if(_connection->fd == -1) {
        uv_tcp_keepalive(&_connection->socket, 1, _connection->keepalive);
        uv_ip4_addr(_connection->hostname.c_str(), _connection->port, &_connection->dest_addr);

        int error = uv_tcp_connect(con, &_connection->socket, (sockaddr*)&_connection->dest_addr, &NetConnectionTask::NetConnectionCb);
        if(error) {
            Resolve(error);
            return false;
        }
    }else{
        int error = uv_tcp_open(&_connection->socket, _connection->fd);
        if(error) {
            Resolve(error);
            return false;
        }

        auto *stream = (uv_stream_t*)&_connection->socket;
        stream->data = _connection.get();
        _connection->is_reading = true;
        error = uv_read_start(stream, &NetAllocCb, &NetReadCb);

        if(error) {
            Resolve(error);
            return false;
        }

        Resolve(IO_SUCCESS);
        return false;
    }

    return true;
}


void NetConnectionTask::NetConnectionCb(uv_connect_t* connection, int status)
{
    assert(connection->data != nullptr);
    auto *task = (NetConnectionTask *)connection->data;

    if (status >= 0) {
        auto *stream = (uv_stream_t*)&task->_connection->socket;
        stream->data = task->_connection.get();
        task->_connection->is_reading = true;
        int error = uv_read_start(stream, &NetAllocCb, &NetReadCb);
        if(!error) {
            task->Resolve(IO_SUCCESS);
            delete task;
        }else{
            task->Resolve(error);
            delete task;
        }
    }else{
        task->Resolve(status);
        delete task;
    }
}


void NetConnectionTask::NetAllocCb(uv_handle_t *handle, size_t size, uv_buf_t *buf)
{
    assert(handle->data != nullptr);
    auto *connection = (TCPConnection*)handle->data;
    if(connection) {
        connection->read_stream.Next(buf, size);
    }
}


void NetConnectionTask::NetReadCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    assert(stream->data != nullptr);
    auto *connection = (TCPConnection*)stream->data;
    if(connection) {
        if(nread >= 0) {
            connection->read_stream.SetLength(connection->read_stream.GetLength() + nread);
        }

        if (connection->read_task != nullptr) {
            if (nread >= 0) {
                *(connection->read_task->_stream) = connection->read_stream;
                connection->read_task->Resolve(IO_SUCCESS);
            }else {
                connection->read_task->Resolve(nread);
            }

            delete connection->read_task;
            connection->read_task = nullptr;
            connection->read_stream.Flush();
        }else{
            if(connection->is_reading && connection->read_stream.GetLength() > MAX_READING_SIZE) {
                connection->is_reading = false;
                uv_read_stop((uv_stream_t *)&connection->socket);
            }
        }
    }
}


bool NetReadTask::Execute(uv_loop_t *loop)
{
    assert(_connection);
    assert(_stream);
    _stream->SetMode(IOStream::R);

    if(!_connection->is_reading) {
        _connection->is_reading = true;

        auto *stream = (uv_stream_t*)&_connection->socket;
        stream->data = _connection.get();
        int error = uv_read_start(stream, &NetConnectionTask::NetAllocCb, &NetConnectionTask::NetReadCb);
        if(error) {
            Resolve(ECANCELED);
            return false;
        }
    }

    if(_connection->read_stream.GetBufferSize() > 0) {
        *_stream = _connection->read_stream;
        _connection->read_stream.Flush();
        Resolve(IO_SUCCESS);
        return false;
    }else{
        if(_connection->read_task == nullptr) {
            _connection->read_task = this;
        }else{
            Resolve(ECANCELED);
            return false;
        }
    }

    return true;
}


bool NetWriteTask::Execute(uv_loop_t *loop)
{
    assert(_connection);
    assert(_stream);
    _stream->SetMode(IOStream::W);
    uv_buf_t *buf = _stream->Next();
    if(buf) {
        auto *write_req = (uv_write_t*)malloc(sizeof(uv_write_t));
        write_req->data = this;
        int error = uv_write(write_req, (uv_stream_t *)&_connection->socket, buf, 1, &NetWriteCb);
        if(error) {
            Resolve(error);
            return false;
        }
        return true;
    }else{
        Resolve(EIO);
        return false;
    }
}


bool NetCloseTask::Execute(uv_loop_t *loop)
{
    uv_tcp_t *tcp_client = &_connection->socket;
    tcp_client->data = this;
    uv_read_stop((uv_stream_t *)&_connection->socket);
    uv_close((uv_handle_t*) tcp_client, &NetCloseCb);
    return true;
}


void NetCloseTask::NetCloseCb(uv_handle_t* handle)
{
    assert(handle->data != nullptr);
    auto *task = (NetCloseTask *)handle->data;
    task->Resolve(IO_SUCCESS);
    delete task;
}


bool NetAddrInfoTask::Execute(uv_loop_t *loop)
{
    assert(_info);
    _info->resolver.data = this;
    int error = uv_getaddrinfo(loop, &_info->resolver, &NetAddrInfoCb, _info->node.c_str(), NULL, &_info->hints);
    if(error) {
        Resolve(error);
        return false;
    }
    return true;
}


void NetAddrInfoTask::NetAddrInfoCb(uv_getaddrinfo_t* req, int status, struct addrinfo* res)
{
    assert(req->data != nullptr);
    auto *task = (NetAddrInfoTask *)req->data;

    if (status >= 0) {
        char addr[17] = {'\0'};
        uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
        task->_info->hostname = std::string {addr};
        task->Resolve(IO_SUCCESS);
        delete task;
    }else{
        task->Resolve(status);
        delete task;
    }
}


bool NetListenTask::Execute(uv_loop_t *loop)
{
    assert(_server);

    int error = 0;
    uv_tcp_t *socket = &_server->socket;
    socket->data = this;
    uv_tcp_init(loop, socket);
    uv_ip4_addr(_server->hostname.c_str(), _server->port, &_server->bind_addr);
    error = uv_tcp_bind(socket, (struct sockaddr*)&_server->bind_addr, 0);
    if (error) {
        Resolve(error);
        return false;
    }

    error = uv_listen((uv_stream_t*) socket, 128 /*backlog*/, &NetListenTask::NetConnectionCb);
    if (error) {
        Resolve(error);
        return false;
    }

    return true;
}


void NetListenTask::NetConnectionCb(uv_stream_t *server, int status)
{
    assert(server->data != nullptr);
    auto task = (NetListenTask* )server->data;

    TCPSessionPtr session = std::make_shared<TCPSession>(server, task->_callback);
    session->Accept();
    session->Run();
}


bool NetUDPBindTask::Execute(uv_loop_t *loop)
{
    assert(_udp);
    uv_udp_init(loop, &_udp->socket);
    uv_ip4_addr(_udp->hostname.c_str(), _udp->port, &_udp->sock_addr);
    int error = uv_udp_bind(&_udp->socket, (struct sockaddr*)&_udp->sock_addr, _flags);

    if(_broadcast)
        uv_udp_set_broadcast(&_udp->socket, 1);

    if (!error) {
        Resolve(IO_SUCCESS);
    }else{
        Resolve(error);
    }

    auto *socket = &_udp->socket;
    socket->data = _udp.get();
    error = uv_udp_recv_start(socket, &NetRecvTask::NetAllocCb, &NetRecvTask::NetRecvCb);
    if(error) {
        Resolve(ECANCELED);
    }

    return false;
}


bool NetSendTask::Execute(uv_loop_t *loop)
{
    assert(_udp);
    auto* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
    req->data = this;

    _stream->SetMode(IOStream::W);
    uv_buf_t *buf = _stream->Next();
    if(buf) {
        uv_ip4_addr(_addr.ip.c_str(), _addr.port, &_send_addr);
        int error = uv_udp_send(req, &_udp->socket, buf, 1, (struct sockaddr *) &_send_addr, NetSendTask::NetSendCb);
        if (error) {
            Resolve(error);
            return false;
        }
    }else{
        Resolve(EIO);
        return false;
    }

    return true;
}


void NetSendTask::NetSendCb(uv_udp_send_t *req, int status)
{
    assert(req->data != nullptr);
    auto task = (NetSendTask* )req->data;
    if (status < 0) {
        task->Resolve(status);
    }else{
        task->Resolve(IO_SUCCESS);
    }

    free(req);
    delete task;
}


bool NetRecvTask::Execute(uv_loop_t *loop)
{
    assert(_udp);
    _stream->SetMode(IOStream::R);

    if(!_udp->all_recv_data.empty()) {
        Resolve(IO_SUCCESS);
        return false;
    }

    auto *socket = &_udp->socket;
    socket->data = _udp.get();
    _udp->recv_task = this;
    uv_udp_recv_start(socket, &NetRecvTask::NetAllocCb, &NetRecvTask::NetRecvCb);

    return true;
}


void NetRecvTask::NetAllocCb(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}


void NetRecvTask::NetRecvCb(uv_udp_t* handle,
                            ssize_t nread,
                            const uv_buf_t* buf,
                            const struct sockaddr* addr,
                            unsigned flags)
{
    assert(handle->data != nullptr);
    auto *udp = (UDP*)handle->data;

    if(nread > 0) {
        UDPReceivedData data = {};
        data.buf = (char *) malloc(nread);
        data.size = nread;
        memcpy(data.buf, buf->base, nread);
        data.addr = addr;

        udp->all_recv_data.push_back(data);
    }

    if(udp->recv_task != nullptr) {
        if (nread >= 0) {
            udp->recv_task->Resolve(IO_SUCCESS);
        }else {
            udp->recv_task->Resolve(nread);
        }

        delete udp->recv_task;
        udp->recv_task = nullptr;
    }else{
        uv_udp_recv_stop(&udp->socket);
    }

    free(buf->base);
}


void FsOpenTask::FsOpenCb(uv_fs_s* req)
{
    auto *task = (FsOpenTask*)req->data;
    assert(req == &task->_request);
    assert(req->fs_type == UV_FS_OPEN);

    task->Resolve(req->result);

    uv_fs_req_cleanup(req);
    assert(req->path == nullptr);
    delete task;
}


void FsCloseTask::FsCloseCb(uv_fs_s* req)
{
    auto task = (FsCloseTask *)(req->data);
    assert(req == &task->_request);
    assert(req->fs_type == UV_FS_CLOSE);
    task->Resolve(IO_SUCCESS);
    delete task;
}


void FsReadTask::FsReadCb(uv_fs_s* req)
{
    auto *task = (FsReadTask*)req->data;
    const auto& stream = task->_stream;
    assert(req == &task->_request);
    assert(req->fs_type == UV_FS_READ);

    if (req->result == 0) {
        stream->Begin();
        task->Resolve(IO_SUCCESS);
        delete task;
        uv_fs_req_cleanup(req);
    } else if (req->result < 0) {
        stream->Begin();
        task->Resolve(req->result);
        delete task;
        uv_fs_req_cleanup(req);
    } else {
        stream->SetLength(stream->GetLength() + req->result);
        uv_buf_t *buf = stream->Next();

        int offset = -1;
        int seek = task->_seek;
        if(seek >= 0) {
            offset = seek + stream->GetBufferSize();
        }

        uv_fs_read(req->loop, &task->_request, task->_fd, buf, 1, offset, FsReadCb);
    }
}


void FsWriteTask::FsWriteCb(uv_fs_s *req)
{
    auto *task = (FsWriteTask*)req->data;
    const auto& stream = task->_stream;
    assert(req == &task->_request);
    assert(req->fs_type == UV_FS_WRITE);

    if (req->result == 0) {
        stream->Begin();
        task->Resolve(IO_SUCCESS);
        delete task;
        uv_fs_req_cleanup(req);
    } else if (req->result < 0) {
        stream->Begin();
        task->Resolve(req->result);
        delete task;
        uv_fs_req_cleanup(req);
    } else {
        uv_buf_t *buf = stream->Next();

        if(buf) {
            uv_fs_write(req->loop, &task->_request, task->_fd, buf, 1, -1, FsWriteCb);
        }else{
            stream->Begin();
            task->Resolve(IO_SUCCESS);
            delete task;
            uv_fs_req_cleanup(req);
        }
    }
}


void NetWriteTask::NetWriteCb(uv_write_t* req, int status)
{
    auto *task = (NetWriteTask* )req->data;
    assert(req->type == UV_WRITE);

    if (status >= 0) {
        task->Resolve(IO_SUCCESS);
    }else{
        task->Resolve(status);
    }
    delete task;
    free(req);
}