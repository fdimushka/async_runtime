#ifndef AR_SERVER_H
#define AR_SERVER_H


#include "ar/task.hpp"
#include "ar/coroutine.hpp"
#include "stream.hpp"
#include "uv.h"


namespace AsyncRuntime {
    struct TCPServer {
        std::string                     hostname;
        int                             port;
        struct sockaddr_in              bind_addr;
        uv_tcp_t                        socket;
    };


    typedef std::shared_ptr<TCPServer>  TCPServerPtr;

    class IOTask;
    class NetReadTask;


//    struct IONetReader {
//        IOStream stream;
//        IOTask *task;
//        std::mutex  mutex;
//
//        void SetReadTask(IOTask *task_);
//    };


    struct TCPConnection {
        int                             fd;
        std::string                     hostname;
        int                             port;
        int                             keepalive;
        struct sockaddr_in              dest_addr;
        uv_tcp_t                        socket;
        uv_connect_t                    connect;
        IOStream                        read_stream;
        bool                            is_reading;
        NetReadTask                     *read_task;
    };


    typedef std::shared_ptr<TCPConnection>  TCPConnectionPtr;


    struct NetAddrInfo {
        std::string                     node;
        std::string                     hostname;
        struct addrinfo                 hints;
        uv_getaddrinfo_t                resolver;
    };

    typedef std::shared_ptr<NetAddrInfo>    NetAddrInfoPtr;

    class TCPSession : public std::enable_shared_from_this<TCPSession> {
    public:
        typedef std::function<void(CoroutineHandler*, TCPConnectionPtr)> CallbackType;

        explicit TCPSession(uv_stream_t *server, const CallbackType & callback);
        ~TCPSession();

        void Run();


        bool IsAccepted() const { return accepted_; }
        void Accept();


        uv_tcp_t* GetClient() { return client_; }
        const uv_tcp_t* GetClient() const { return client_; }


        uv_stream_t* GetServer() { return server_; }
        const uv_stream_t* GetServer() const { return server_; }


        void Invoke(CoroutineHandler *handler);
    private:
        static void Session(CoroutineHandler *handler, YieldVoid yield, std::shared_ptr<TCPSession> session);


        uv_loop_t                                           *loop_;
        uv_stream_t                                         *server_;
        uv_tcp_t                                            *client_;
        CallbackType                                        fn;
        std::shared_ptr<Coroutine<void>>                    coroutine;
        TCPConnectionPtr                                    connection_;
        bool                                                accepted_;
    };


    typedef std::shared_ptr<TCPSession>  TCPSessionPtr;


    /**
     * @brief
     * @param hostname
     * @param port
     * @param keepalive
     * @return
     */
    TCPConnectionPtr MakeTCPConnection(const char* hostname, int port, int keepalive = 60);
    TCPConnectionPtr MakeTCPConnection(int fd);


    /**
     * @brief create tcp server;
     * @param hostname
     * @param port
     * @return
     */
    TCPServerPtr MakeTCPServer(const char* hostname, int port);


    /**
     * @brief
     * @param addrname
     * @return
     */
    NetAddrInfoPtr MakeNetAddrInfo(const char* node);
}

#endif //AR_SERVER_H
