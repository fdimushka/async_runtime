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


    struct IOFsClose {};


    struct IONetListen {
        int                               flags = 0;
        TCPSession::HandlerType           handle_connection;
    };


    struct IONetConnect {};
    struct IONetRead { uv_tcp_t *socket; };
    struct IONetWrite { uv_tcp_t *socket; };
    struct IONetClose { uv_tcp_t *socket; };


    typedef int                                     IOResult;
    typedef std::shared_ptr<Result<IOResult>>       IOResultPtr;


#define IO_SUCCESS 0


    void FsOpenCb(uv_fs_s* req);
    void FsReadCb(uv_fs_s* req);
    void FsWriteCb(uv_fs_s* req);
    void FsCloseCb(uv_fs_s* req);

    void NetConnectionCb(uv_stream_t *server, int status);
    void NetConnectionCb(uv_connect_t* connection, int status);
    void NetAllocCb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
    void NetReadCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    void NetWriteCb(uv_write_t* req, int status);
    void NetCloseCb(uv_handle_t* handle);

    const char* FSErrorMsg(int error);
    const char* FSErrorName(int error);


    class IOTask : public Task {
    public:
        void Execute(const ExecutorState& executor_) override { throw std::runtime_error("not implemented!"); }
        virtual bool Execute(uv_loop_t *loop) = 0;
    };


    template<typename Method>
    class IOFsTaskImpl : public IOTask
    {
    public:
        explicit IOFsTaskImpl(Method m, const IOStreamPtr& s) :
                method(m),
                stream(s),
                result(new Result<IOResult>() ) {
            assert(stream);
        };


        bool Execute(uv_loop_t *loop) {
            request.data = this;
            return CallMethod(loop);
        }


        void Resolve(IOResult res) {
            result->SetValue(res);
        }


        std::shared_ptr<Result<IOResult>> GetResult() { return result; }
        const uv_fs_s* GetRequest() const { return &request; }
        uv_fs_s* GetRequest() { return &request; }
        const IOStreamPtr& GetStream() const { return stream; }
        const Method& GetMethod() const { return method; }
    private:
        bool CallMethod(uv_loop_t *loop);


        std::shared_ptr<Result<IOResult>>                        result;
        IOStreamPtr                                              stream;
        Method                                                   method;
        uv_fs_t                                                  request;
    };


    template<typename Method>
    class IONetTaskImpl : public IOTask
    {
    public:
        explicit IONetTaskImpl(Method m, const IOStreamPtr& s) :
                method(m),
                stream(s),
                result(new Result<IOResult>() ) {
        };


        explicit IONetTaskImpl(Method m) :
                method(m),
                result(new Result<IOResult>() ) {
        };


        bool Execute(uv_loop_t *loop) {
            return CallMethod(loop);
        }


        void Resolve(IOResult res) {
            result->SetValue(res);
        }


        std::shared_ptr<Result<IOResult>> GetResult() { return result; }
        const Method& GetMethod() const { return method; }
        const IOStreamPtr& GetStream() const { return stream; }
    private:
        bool CallMethod(uv_loop_t *loop);

        std::shared_ptr<Result<IOResult>>                        result;
        Method                                                   method;
        IOStreamPtr                                              stream;
    };


    template<>
    class IONetTaskImpl<IONetListen> : public IOTask {
    public:
        explicit IONetTaskImpl(IONetListen m, const TCPServerPtr& s) :
                method(m),
                server(s),
                result(new Result<IOResult>() ) {
        };


        bool Execute(uv_loop_t *loop) {
            return CallMethod(loop);
        }


        void Resolve(IOResult res) {
            result->SetValue(res);
        }


        std::shared_ptr<Result<IOResult>> GetResult() { return result; }
        const IONetListen& GetMethod() const { return method; }
    private:
        inline bool CallMethod(uv_loop_s *loop);

        TCPServerPtr                                             server;
        std::shared_ptr<Result<IOResult>>                        result;
        IONetListen                                              method;
    };


    template<>
    class IONetTaskImpl<IONetConnect> : public IOTask {
    public:
        explicit IONetTaskImpl(IONetConnect m, const TCPConnectionPtr& c) :
                method(m),
                connection(c),
                result(new Result<IOResult>() ) {
        };


        bool Execute(uv_loop_t *loop) {
            return CallMethod(loop);
        }


        void Resolve(IOResult res) {
            result->SetValue(res);
        }


        std::shared_ptr<Result<IOResult>> GetResult() { return result; }
        const IONetConnect& GetMethod() const { return method; }
    private:
        inline bool CallMethod(uv_loop_s *loop);

        TCPConnectionPtr                                         connection;
        std::shared_ptr<Result<IOResult>>                        result;
        IONetConnect                                             method;
    };


    template<>
    inline bool AsyncRuntime::IOFsTaskImpl<AsyncRuntime::IOFsOpen>::CallMethod(uv_loop_s *loop) {
        uv_fs_open(loop, &request, method.filename, method.flags, method.mode, FsOpenCb);
        return true;
    }


    template<>
    inline bool AsyncRuntime::IOFsTaskImpl<AsyncRuntime::IOFsClose>::CallMethod(uv_loop_s *loop) {
        uv_file fd = stream->GetFd();
        uv_fs_close(loop, &request, fd, FsCloseCb);
        return true;
    }


    template<>
    inline bool AsyncRuntime::IOFsTaskImpl<AsyncRuntime::IOFsRead>::CallMethod(uv_loop_s *loop) {
        stream->SetMode(IOStream::R);
        uv_buf_t *buf = stream->Next();
        uv_file fd = stream->GetFd();
        uv_fs_read(loop, &request, fd, buf, 1, method.seek, FsReadCb);
        return true;
    }


    template<>
    inline bool AsyncRuntime::IOFsTaskImpl<AsyncRuntime::IOFsWrite>::CallMethod(uv_loop_s *loop) {
        stream->SetMode(IOStream::W);
        uv_file fd = stream->GetFd();
        uv_buf_t *buf = stream->Next();
        if(buf) {
            uv_fs_write(loop, &request, fd, buf, 1, method.seek, FsWriteCb);
            return true;
        }else{
            Resolve(EIO);
            return false;
        }
    }


    inline bool AsyncRuntime::IONetTaskImpl<AsyncRuntime::IONetListen>::CallMethod(uv_loop_s *loop) {
        assert(server);
        int error = 0;
        uv_tcp_t *tcp_server = &server->server;
        tcp_server->data = this;
        uv_tcp_init(loop, tcp_server);
        uv_ip4_addr(server->hostname.c_str(), server->port, &server->bind_addr);
        error = uv_tcp_bind(tcp_server, (struct sockaddr*)&server->bind_addr, 0);
        if (error) {
            Resolve(error);
            return false;
        }

        error = uv_listen((uv_stream_t*) tcp_server, 128 /*backlog*/, NetConnectionCb);
        if (error) {
            Resolve(error);
            return false;
        }

        return true;
    }


    inline bool AsyncRuntime::IONetTaskImpl<AsyncRuntime::IONetConnect>::CallMethod(uv_loop_s *loop) {
        assert(connection);
        uv_connect_t *con = &connection->connect;
        con->data = this;
        uv_tcp_init(loop, &connection->socket);
        uv_tcp_keepalive(&connection->socket, 1, connection->keepalive);
        uv_ip4_addr(connection->hostname.c_str(), connection->port, &connection->dest_addr);
        int error = uv_tcp_connect(con, &connection->socket, (sockaddr*)&connection->dest_addr, NetConnectionCb);
        if(error) {
            Resolve(error);
            return false;
        }

        return true;
    }


    template<>
    inline bool AsyncRuntime::IONetTaskImpl<AsyncRuntime::IONetRead>::CallMethod(uv_loop_s *loop) {
        assert(stream);
        stream->SetMode(IOStream::R);
        uv_tcp_t *tcp_client = method.socket;
        tcp_client->data = this;
        int error = uv_read_start((uv_stream_t*) tcp_client, NetAllocCb, NetReadCb);
        if(error) {
            Resolve(error);
            return false;
        }

        return true;
    }


    template<>
    inline bool AsyncRuntime::IONetTaskImpl<AsyncRuntime::IONetWrite>::CallMethod(uv_loop_s *loop) {
        assert(stream);
        stream->SetMode(IOStream::W);
        auto *client = (uv_stream_t *)method.socket;
        uv_buf_t *buf = stream->Next();
        if(buf) {
            auto *write_req = (uv_write_t*)malloc(sizeof(uv_write_t));
            write_req->data = this;
            int error = uv_write(write_req, client, buf, 1, NetWriteCb);
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


    template<>
    inline bool AsyncRuntime::IONetTaskImpl<AsyncRuntime::IONetClose>::CallMethod(uv_loop_s *loop) {
        uv_tcp_t *tcp_client = method.socket;
        tcp_client->data = this;
        uv_close((uv_handle_t*) tcp_client, NetCloseCb);
        return true;
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
