#include "ar/io_task.hpp"


using namespace AsyncRuntime;


const char* AsyncRuntime::FSErrorMsg(int error)
{
    return uv_strerror(error);
}


const char* AsyncRuntime::FSErrorName(int error)
{
    return uv_err_name(error);
}


void AsyncRuntime::FsOpenCb(uv_fs_s* req)
{
    auto task = IOFsTaskCast<AsyncRuntime::IOFsOpen>(req->data);
    const auto& stream = task->GetStream();
    assert(req == task->GetRequest());
    assert(req->fs_type == UV_FS_OPEN);

    uv_file fd = req->result;
    stream->SetFd(fd);
    stream->Begin();

    if(req->result < 0) {
        task->Resolve(req->result);
    }else{
        task->Resolve(IO_SUCCESS);
    }

    uv_fs_req_cleanup(req);
    assert(req->path == nullptr);
    delete task;
}


void AsyncRuntime::FsCloseCb(uv_fs_s* req)
{
    auto task = IOFsTaskCast<AsyncRuntime::IOFsClose>(req->data);
    const auto& stream = task->GetStream();
    assert(req == task->GetRequest());
    assert(req->fs_type == UV_FS_CLOSE);

    stream->SetFd(-1);
    stream->Begin();
    task->Resolve(IO_SUCCESS);
    delete task;
}


void AsyncRuntime::FsReadCb(uv_fs_s* req)
{
    auto *task = IOFsTaskCast<AsyncRuntime::IOFsRead>(req->data);
    const auto& stream = task->GetStream();
    assert(req == task->GetRequest());
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
        stream->length += req->result;
        uv_buf_t *buf = stream->Next();
        uv_file fd = stream->GetFd();

        int offset = -1;
        int seek = task->GetMethod().seek;
        if(seek >= 0) {
            offset = seek + stream->GetBufferSize();
        }

        uv_fs_read(req->loop, task->GetRequest(), fd, buf, 1, offset, FsReadCb);
    }
}


void AsyncRuntime::FsWriteCb(uv_fs_s *req)
{
    auto task = IOFsTaskCast<AsyncRuntime::IOFsWrite>(req->data);
    const auto& stream = task->GetStream();
    assert(req == task->GetRequest());
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
        uv_file fd = stream->GetFd();
        uv_buf_t *buf = stream->Next();

        if(buf) {
            uv_fs_write(req->loop, task->GetRequest(), fd, buf, 1, -1, FsWriteCb);
        }else{
            stream->Begin();
            task->Resolve(IO_SUCCESS);
            delete task;
            uv_fs_req_cleanup(req);
        }
    }
}


void AsyncRuntime::NetConnectionCb(uv_stream_t *server, int status)
{
    assert(server->data != nullptr);
    auto task = IONetTaskCast<AsyncRuntime::IONetListen>(server->data);
    auto &method = task->GetMethod();

    TCPSessionPtr session = std::make_shared<TCPSession>(server, method.handle_connection);
    session->Accept();
    session->Run();
}


void AsyncRuntime::NetConnectionCb(uv_connect_t* connection, int status)
{
    assert(connection->data != nullptr);
    auto task = IONetTaskCast<AsyncRuntime::IONetConnect>(connection->data);

    if (status >= 0) {
        task->Resolve(IO_SUCCESS);
        delete task;
    }else{
        task->Resolve(status);
        delete task;
    }
}


void AsyncRuntime::NetAllocCb(uv_handle_t *handle, size_t size, uv_buf_t *buf)
{
    assert(handle->data != nullptr);
    auto *task = IONetTaskCast<AsyncRuntime::IONetRead>(handle->data);
    task->GetStream()->Next(buf, size);
}


void AsyncRuntime::NetReadCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    assert(stream->data != nullptr);
    auto *task = IONetTaskCast<AsyncRuntime::IONetRead>(stream->data);
    auto &io_stream = task->GetStream();
    uv_read_stop(stream);

    if (nread >= 0) {
        io_stream->length += nread;
    }
//    else if (nread == UV_EOF) {
//
//    }

    io_stream->Begin();
    task->Resolve(IO_SUCCESS);
    delete task;
}


void AsyncRuntime::NetWriteCb(uv_write_t* req, int status)
{
    auto task = IONetTaskCast<AsyncRuntime::IONetWrite>(req->data);
    assert(req->type == UV_WRITE);
    task->Resolve(IO_SUCCESS);
    delete task;
    free(req);
}


void AsyncRuntime::NetCloseCb(uv_handle_t* handle)
{
    assert(handle->data != nullptr);
    auto *task = IONetTaskCast<AsyncRuntime::IONetClose>(handle->data);
    task->Resolve(IO_SUCCESS);
    delete task;
}


void AsyncRuntime::NetAddrInfoCb(uv_getaddrinfo_t* req, int status, struct addrinfo* res)
{
    assert(req->data != nullptr);
    auto task = IONetTaskCast<AsyncRuntime::IONetAddrInfo>(req->data);

    if (status >= 0) {
        char addr[17] = {'\0'};
        uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
        task->GetInfo()->hostname = std::string {addr};
        task->Resolve(IO_SUCCESS);
        delete task;
    }else{
        task->Resolve(status);
        delete task;
    }
}