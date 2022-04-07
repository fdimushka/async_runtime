#ifndef AR_IO_TASK_H
#define AR_IO_TASK_H


#include "ar/task.hpp"
#include "ar/stream.hpp"

#include "uv.h"

namespace AsyncRuntime {
    struct IOFsOpen {
        const char* filename;
        int         flags;
        int         mode;
    };


    struct IOFsRead {
        int64_t seek = -1;
        int64_t size = 0;
    };


    struct IOFsWrite {
        int64_t seek = -1;
    };


    struct IOFsClose { };


    typedef int                                     IOResult;
    typedef std::shared_ptr<Result<IOResult>>       IOResultPtr;


#define IO_SUCCESS 0


    void FsOpenCb(uv_fs_s* req);
    void FsReadCb(uv_fs_s* req);
    void FsWriteCb(uv_fs_s* req);
    void FsCloseCb(uv_fs_s* req);


    const char* FSErrorMsg(int error);
    const char* FSErrorName(int error);


    template<typename Method>
    class IOFsTaskImpl : public Task
    {
    public:
        explicit IOFsTaskImpl(Method m, IOFsStreamPtr s) :
                Task(),
                method(m),
                stream(std::move(s)),
                result(new Result<IOResult>() ) {
            assert(stream);
        };


        ~IOFsTaskImpl() override = default;


        void Execute(const ExecutorState& executor_) override {
            assert(executor_.data != nullptr);
            request.data = this;
            CallMethod((uv_loop_t*)executor_.data);
        }


        void Resolve(IOResult res) {
            result->SetValue(res);
        }


        std::shared_ptr<Result<IOResult>> GetResult() { return result; }
        const uv_fs_s* GetRequest() const { return &request; }
        uv_fs_s* GetRequest() { return &request; }
        const IOFsStreamPtr& GetStream() const { return stream; }
        const Method& GetMethod() const { return method; }
    private:
        void CallMethod(uv_loop_t *loop);


        std::shared_ptr<Result<IOResult>>                        result;
        IOFsStreamPtr                                            stream;
        Method                                                   method;
        uv_fs_t                                                  request;
    };


    template<>
    inline void AsyncRuntime::IOFsTaskImpl<AsyncRuntime::IOFsOpen>::CallMethod(uv_loop_s *loop) {
        uv_fs_open(loop, &request, method.filename, method.flags, method.mode, FsOpenCb);
    }


    template<>
    inline void AsyncRuntime::IOFsTaskImpl<AsyncRuntime::IOFsClose>::CallMethod(uv_loop_s *loop) {
        uv_file fd = stream->GetFd();
        uv_fs_close(loop, &request, fd, FsCloseCb);
    }


    template<>
    inline void AsyncRuntime::IOFsTaskImpl<AsyncRuntime::IOFsRead>::CallMethod(uv_loop_s *loop) {
        uv_buf_t *buf = stream->Next();
        uv_file fd = stream->GetFd();
        uv_fs_read(loop, &request, fd, buf, 1, method.seek, FsReadCb);
    }


    template<>
    inline void AsyncRuntime::IOFsTaskImpl<AsyncRuntime::IOFsWrite>::CallMethod(uv_loop_s *loop) {
        uv_file fd = stream->GetFd();
        uv_buf_t *buf = stream->Next();
        if(buf) {
            uv_fs_write(loop, &request, fd, buf, 1, method.seek, FsWriteCb);
        }else{
            Resolve(EIO);
        }
    }


    template<typename Method>
    IOFsTaskImpl<Method>* IOFsTaskCast(void *task) {
        return static_cast<IOFsTaskImpl<Method>*>(task);
    }
}

#endif //AR_IO_TASK_H
