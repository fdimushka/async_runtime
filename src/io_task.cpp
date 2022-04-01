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
        stream->IncreaseReadBufferLength(req->result);
        uv_buf_t *buf = stream->CreateReadBuffer();
        uv_file fd = stream->GetFd();

        //@todo: need to implement seek
//        int offset = task->GetMethod().seek;
//
//        if(offset >= 0) {
//            offset = task->GetMethod().seek + stream->GetReadBufferSize();
//        }

        uv_fs_read(req->loop, task->GetRequest(), fd, buf, 1, -1, FsReadCb);
    }
}
