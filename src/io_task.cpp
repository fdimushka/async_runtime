#include "ar/io_task.h"


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