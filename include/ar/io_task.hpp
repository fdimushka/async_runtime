#ifndef AR_IO_TASK_HPP
#define AR_IO_TASK_HPP


#include "ar/task.hpp"
#include "ar/stream.hpp"
#include "ar/tcp.hpp"

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


    struct TCPListen {
        int                                       flags = 0;
        std::function<void(TCPSessionPtr)>        handle_connection;
    };


    struct NetRead {};
    struct NetClose {};


    typedef int                                     IOResult;
    typedef std::shared_ptr<Result<IOResult>>       IOResultPtr;


#define IO_SUCCESS 0


    void FsOpenCb(uv_fs_s* req);
    void FsReadCb(uv_fs_s* req);
    void FsWriteCb(uv_fs_s* req);
    void FsCloseCb(uv_fs_s* req);

    void NetConnectionCb(uv_stream_t *server, int status);
    void NetAllocCb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
    void NetReadCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    void NetCloseCb(uv_handle_t* handle);

    const char* FSErrorMsg(int error);
    const char* FSErrorName(int error);


    class IOTask : public Task {
    public:
        void Execute(const ExecutorState& executor_) override { throw std::runtime_error("not implemented!"); }
        virtual void Execute(uv_loop_t *loop) = 0;

        uv_async_t* async_handle = nullptr;
    };


    template<typename Method>
    class IOFsTaskImpl : public IOTask
    {
    public:
        explicit IOFsTaskImpl(Method m, IOFsStreamPtr s) :
                method(m),
                stream(std::move(s)),
                result(new Result<IOResult>() ) {
            assert(stream);
        };


        void Execute(uv_loop_t *loop) {
            request.data = this;
            CallMethod(loop);
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


    template<typename Method>
    class IONetTaskImpl : public IOTask
    {
    public:
        explicit IONetTaskImpl(Method m, TCPServerPtr s) :
                method(m),
                server(std::move(s)),
                result(new Result<IOResult>() ) {
        };


        explicit IONetTaskImpl(Method m, TCPSessionPtr s, IOFsStreamPtr st) :
                method(m),
                session(std::move(s)),
                stream(std::move(st)),
                result(new Result<IOResult>() ) {
        };


        explicit IONetTaskImpl(Method m, TCPSessionPtr s) :
                method(m),
                session(std::move(s)),
                result(new Result<IOResult>() ) {
        };


        void Execute(uv_loop_t *loop) {
            CallMethod(loop);
        }


        void Resolve(IOResult res) {
            result->SetValue(res);
        }


        std::shared_ptr<Result<IOResult>> GetResult() { return result; }
        const Method& GetMethod() const { return method; }
        const IOFsStreamPtr& GetStream() const { return stream; }
    private:
        void CallMethod(uv_loop_t *loop);


        std::shared_ptr<Result<IOResult>>                        result;
        Method                                                   method;
        TCPServerPtr                                             server;
        TCPSessionPtr                                            session;
        IOFsStreamPtr                                            stream;
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


    template<>
    inline void AsyncRuntime::IONetTaskImpl<AsyncRuntime::TCPListen>::CallMethod(uv_loop_s *loop) {
        assert(server);

        int error = 0;
        uv_tcp_t *tcp_server = &server->server;
        tcp_server->data = this;
        uv_tcp_init(loop, tcp_server);
        uv_ip4_addr(server->hostname.c_str(), server->port, &server->bind_addr);
        error = uv_tcp_bind(tcp_server, (struct sockaddr*)&server->bind_addr, 0);
        if (error) {
            Resolve(error);
            return;
        }

        error = uv_listen((uv_stream_t*) tcp_server, 128 /*backlog*/, NetConnectionCb);
        if (error) {
            Resolve(error);
        }
    }


    template<>
    inline void AsyncRuntime::IONetTaskImpl<AsyncRuntime::NetRead>::CallMethod(uv_loop_s *loop) {
        assert(stream);
        assert(session);

        uv_tcp_t *tcp_client = session->GetClient();
        tcp_client->data = this;
        int error = uv_read_start((uv_stream_t*) tcp_client, NetAllocCb, NetReadCb);
        if(error) {
            Resolve(error);
        }
    }


    template<>
    inline void AsyncRuntime::IONetTaskImpl<AsyncRuntime::NetClose>::CallMethod(uv_loop_s *loop) {
        assert(session);
        uv_tcp_t *tcp_client = session->GetClient();
        tcp_client->data = this;
        uv_close((uv_handle_t*) tcp_client, NetCloseCb);
    }


    template<typename Method>
    IOFsTaskImpl<Method>* IOFsTaskCast(void *task) {
        return static_cast<IOFsTaskImpl<Method>*>(task);
    }


    template<typename Method>
    IONetTaskImpl<Method>* IONetTaskCast(void *task) {
        return static_cast<IONetTaskImpl<Method>*>(task);
    }
}

#endif //AR_IO_TASK_HPP
