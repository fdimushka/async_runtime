#ifndef AR_IO_TASK_HPP
#define AR_IO_TASK_HPP


#include "ar/task.hpp"
#include "ar/stream.hpp"
#include "ar/net.hpp"
#include "ar/stream_buffer.hpp"

#include "uv.h"


namespace AsyncRuntime {
    typedef int                                     IOResult;
    typedef std::shared_ptr<Result<IOResult>>       IOResultPtr;


#define IO_SUCCESS 0


    const char* FSErrorMsg(int error);
    const char* FSErrorName(int error);


    /**
     * @brief base i/o task
     * @class IOTask
     */
    class IOTask : public Task {
    public:
        IOTask() : result(new Result<IOResult>() ) { }
        void Execute(const ExecutorState& executor_) override { throw std::runtime_error("not implemented!"); }
        virtual bool Execute(uv_loop_t *loop) = 0;


        void Resolve(IOResult res) { result->SetValue(res); }
        std::shared_ptr<Result<IOResult>> GetResult() { return result; }
    private:
        std::shared_ptr<Result<IOResult>>                        result;
    };


    /**
     * @brief Open file descriptor task
     * @class FsOpenTask
     */
    class FsOpenTask : public IOTask {
    public:
        FsOpenTask(const char* filename, int flags, int mode) : _filename(filename), _flags(flags), _mode(mode), _request{} { }
        bool Execute(uv_loop_t *loop) override;
        static void FsOpenCb(uv_fs_s* req);
    private:
        const char*         _filename;
        int                 _flags;
        int                 _mode;
        uv_fs_t             _request;
    };


    /**
     * @brief Read from file descriptor task
     * @class FsReadTask
     */
    class FsReadTask : public IOTask {
    public:
        FsReadTask(int fd, const IOStreamPtr& stream, int seek = 0):_fd(fd), _stream(stream), _seek(seek), _request{} {}
        bool Execute(uv_loop_t *loop) override;
        static void FsReadCb(uv_fs_s* req);
    private:
        int                 _fd;
        int                 _seek;
        IOStreamPtr         _stream;
        uv_fs_t             _request;
    };


    /**
     * @brief Write to file descriptor task
     * @class FsWriteTask
     */
    class FsWriteTask : public IOTask {
    public:
        FsWriteTask(int fd, const IOStreamPtr& stream, int seek = 0):_fd(fd), _stream(stream), _seek(seek), _request{} {}
        bool Execute(uv_loop_t *loop) override;
        static void FsWriteCb(uv_fs_s* req);
    private:
        int                 _fd;
        int                 _seek;
        IOStreamPtr         _stream;
        uv_fs_t             _request;
    };


    /**
     * @brief Close file descriptor task
     * @class FsCloseTask
     */
    class FsCloseTask : public IOTask {
    public:
        FsCloseTask(int fd):_fd(fd), _request{} {}
        bool Execute(uv_loop_t *loop) override;
        static void FsCloseCb(uv_fs_s* req);
    private:
        int                 _fd;
        uv_fs_t             _request;
    };


    /**
     * @brief Open socket task
     * @class NetOpenTask
     */
    class NetConnectionTask : public IOTask {
    public:
        NetConnectionTask(const TCPConnectionPtr& connection) : _connection(connection) { }
        bool Execute(uv_loop_t *loop) override;
        static void NetConnectionCb(uv_connect_t* connection, int status);
        static void NetAllocCb(uv_handle_t *handle, size_t size, uv_buf_t *buf);
        static void NetReadCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    private:
        TCPConnectionPtr     _connection;
    };


    /**
     * @brief Read from socket task
     * @class NetReadTask
     */
    class NetReadTask : public IOTask {
    public:
        NetReadTask(const TCPConnectionPtr& connection, const std::shared_ptr<StreamBuffer<>>& stream_buffer): _connection(connection), _stream_buffer(stream_buffer) {}
        bool Execute(uv_loop_t *loop) override;
    private:
        TCPConnectionPtr                      _connection;
        std::shared_ptr<StreamBuffer<>>       _stream_buffer;
    };


    /**
     * @brief write to socket task
     * @class NetWriteTask
     */
    class NetWriteTask : public IOTask {
    public:
        NetWriteTask(const TCPConnectionPtr& connection, const char* buffer, size_t size);
        bool Execute(uv_loop_t *loop) override;
        static void NetWriteCb(uv_write_t* req, int status);
    private:
        TCPConnectionPtr     _connection;
        StreamBuffer<>       _stream_buffer;
    };


    /**
     * @brief close the socket task
     * @class NetCloseTask
     */
    class NetCloseTask : public IOTask {
    public:
        NetCloseTask(const TCPConnectionPtr& connection): _connection(connection) {}
        bool Execute(uv_loop_t *loop) override;
        static void NetCloseCb(uv_handle_t* handle);
    private:
        TCPConnectionPtr     _connection;
    };


    /**
     * @class IONetAddrInfo
     * @brief dns task
     */
    class NetAddrInfoTask : public IOTask {
    public:
        NetAddrInfoTask(const NetAddrInfoPtr& info): _info(info) {}
        bool Execute(uv_loop_t *loop) override;
        static void NetAddrInfoCb(uv_getaddrinfo_t* req, int status, struct addrinfo* res);
    private:
        NetAddrInfoPtr    _info;
    };


    /**
     * @class IONetListen
     * @brief Listen
     */
    class NetListenTask : public IOTask {
    public:
        typedef std::function<void(CoroutineHandler*, const TCPConnectionPtr&)> CallbackType;
        NetListenTask(const TCPServerPtr& server, const CallbackType& callback): _server(server), _callback(callback) {}
        bool Execute(uv_loop_t *loop) override;
        static void NetConnectionCb(uv_stream_t *server, int status);

        TCPServerPtr    _server;
    private:
        CallbackType    _callback;
    };


    /**
     * @class NetUDPBindTask
     * @brief Listen
     */
    class NetUDPBindTask : public IOTask {
    public:
        NetUDPBindTask(const UDPPtr &udp, int flags, bool broadcast = false): _udp(udp), _broadcast(broadcast), _flags(flags) {}
        bool Execute(uv_loop_t *loop) override;
    private:
        int       _flags;
        bool      _broadcast;
        UDPPtr    _udp;
    };


    /**
     * @brief send to socket task
     * @class NetSendTask
     */
    class NetSendTask : public IOTask {
    public:
        NetSendTask(const UDPPtr &udp, const IOStreamPtr& stream, const IPv4Addr& addr): _udp(udp), _stream(stream), _addr(addr) {}
        bool Execute(uv_loop_t *loop) override;
        static void NetSendCb(uv_udp_send_t *req, int status);
    private:
        UDPPtr               _udp;
        IPv4Addr             _addr;
        struct sockaddr_in   _send_addr;
        IOStreamPtr          _stream;
    };


    /**
     * @brief
     * @class NetRecvTask
     */
    class NetRecvTask : public IOTask {
    public:
        NetRecvTask(const UDPPtr &udp, const IOStreamPtr& stream): _udp(udp), _stream(stream) {}
        bool Execute(uv_loop_t *loop) override;
        static void NetAllocCb(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf);
        static void NetRecvCb(uv_udp_t* handle,
                              ssize_t nread,
                              const uv_buf_t* buf,
                              const struct sockaddr* addr,
                              unsigned flags);
    private:
        UDPPtr               _udp;
        IOStreamPtr          _stream;
    };
}

#endif //AR_IO_TASK_HPP
