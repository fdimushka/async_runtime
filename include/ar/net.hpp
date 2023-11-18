#ifndef AR_SERVER_H
#define AR_SERVER_H


#include "ar/task.hpp"
#include "ar/coroutine.hpp"
#include "ar/stream_buffer.hpp"
#include "stream.hpp"
#include "uv.h"


namespace AsyncRuntime {
    struct IPv4Addr {
        std::string                     ip;
        int                             port;
    };


    struct TCPServer {
        std::string                     hostname;
        int                             port;
        struct sockaddr_in              bind_addr;
        uv_tcp_t                        socket;
        std::function<void(void)>       on_bind_success;
        std::function<void(int)>        on_bind_error;
    };


    typedef std::shared_ptr<TCPServer>  TCPServerPtr;

    class IOTask;
    class NetReadTask;
    class NetRecvTask;


    struct TCPConnection {
        int                             fd;
        std::string                     hostname;
        int                             port;
        int                             keepalive;
        struct sockaddr_in              dest_addr;
        uv_tcp_t                        socket;
        uv_connect_t                    connect;
        std::atomic_int                 read_error = {0};
        StreamBuffer<>                  read_stream;
        std::shared_ptr<AsyncRuntime::Result<int>>  read_result;
        std::mutex mutex;
    };

    typedef std::shared_ptr<TCPConnection>  TCPConnectionPtr;

    void CloseReadStream(TCPConnection *conn, int error);
    void ProduceReadStream(TCPConnection *conn, char* buffer, size_t size);
    int ConsumeReadStream(TCPConnection *conn, char* buffer, size_t size);
    int ConsumeReadStream(const std::shared_ptr<TCPConnection> & conn, char* buffer, size_t size);
    std::shared_ptr<AsyncRuntime::Result<int>> AsyncWaitReadStream(const std::shared_ptr<TCPConnection> & conn, size_t size);

    struct UDPReceivedData {
        const struct sockaddr*          addr;
        char*                           buf;
        ssize_t                         size;
    };


    void FlushUDPReceivedData(std::vector<UDPReceivedData>& all_recv_data);


    struct UDP {
        int                             fd;
        std::string                     hostname;
        int                             port;
        struct sockaddr_in              sock_addr;
        uv_udp_t                        socket;
        NetRecvTask*                    recv_task;
        std::vector<UDPReceivedData>    all_recv_data;
    };


    typedef std::shared_ptr<UDP>  UDPPtr;


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

        explicit TCPSession(uv_stream_t *server, CallbackType  callback);
        ~TCPSession();

        void Run();


        bool IsAccepted() const { return accepted_; }
        void Accept();


        uv_tcp_t* GetClient() { return client_; }
        const uv_tcp_t* GetClient() const { return client_; }


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
    TCPConnectionPtr MakeTCPConnection(const char* hostname, int port, int keepalive = 10);
    TCPConnectionPtr MakeTCPConnection(int fd);


    UDPPtr MakeUDP(const char* hostname, int port);


    /**
     * @brief create tcp server;
     * @param hostname
     * @param port
     * @return
     */
    TCPServerPtr MakeTCPServer(const char* hostname, int port);


    /**
     * @brief
     * @param hostname
     * @param port
     * @param on_bind_success
     * @param on_bind_error
     * @return
     */
    TCPServerPtr MakeTCPServer(const char* hostname,
                               int port, const
                               std::function<void(void)> &on_bind_success,
                               const std::function<void(int)> &on_bind_error);

    /**
     * @brief
     * @param addrname
     * @return
     */
    NetAddrInfoPtr MakeNetAddrInfo(const char* node);
}

#endif //AR_SERVER_H
