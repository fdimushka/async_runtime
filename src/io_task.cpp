#include "ar/io_task.h"


using namespace AsyncRuntime;


void AsyncRuntime::FsOpenCb(uv_fs_s* req)
{
    auto task = IOFsTaskCast<AsyncRuntime::IOFsOpen>(req->data);
    const auto& stream = task->GetStream();
    assert(req == task->GetRequest());
    assert(req->fs_type == UV_FS_OPEN);

    uv_file fd = req->result;
    stream->SetFd(fd);

    if(fd < 0) {
        task->Resolve(EIO);
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
    stream->Flush();
    task->Resolve(IO_SUCCESS);

    delete task;
}


void AsyncRuntime::FsReadCb(uv_fs_s* req)
{
    auto task = IOFsTaskCast<AsyncRuntime::IOFsRead>(req->data);
    const auto& stream = task->GetStream();
    assert(req == task->GetRequest());
    assert(req->fs_type == UV_FS_READ);

    if (req->result <= 0) {
        task->Resolve(EOF);
        delete task;
        uv_fs_req_cleanup(req);
    }else{
        auto &read_stream = stream->GetReadStream();
        read_stream.length += req->result;
        uv_buf_t *buf = read_stream.Next();
        uv_file fd = stream->GetFd();

        int offset = -1;
        int seek = task->GetMethod().seek;
        if(seek >= 0) {
            offset = seek + stream->GetReadBufferSize();
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

    task->Resolve(IO_SUCCESS);

    uv_fs_req_cleanup(req);
    assert(req->path == nullptr);

    delete task;
}