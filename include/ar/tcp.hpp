#ifndef AR_SERVER_H
#define AR_SERVER_H


#include "ar/task.hpp"
#include "ar/coroutine.hpp"
#include "uv.h"


namespace AsyncRuntime {
    struct TCPServer {
        std::string                     hostname;
        int                             port;
        struct sockaddr_in              bind_addr;
        uv_tcp_t                        server;
    };


    typedef std::shared_ptr<TCPServer>  TCPServerPtr;


    struct TCPConnection {
        std::string                     hostname;
        int                             port;
        int                             keepalive;
        struct sockaddr_in              dest_addr;
        uv_tcp_t                        socket;
        uv_connect_t                    connect;
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
        typedef std::function<void(CoroutineHandler* ,std::shared_ptr<TCPSession>)> HandlerType;

        explicit TCPSession(uv_stream_t *server, const HandlerType & connection_handler);
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
        static void Session(CoroutineHandler *handler, YieldVoid yield, const std::shared_ptr<TCPSession>& session);


        uv_loop_t                                           *loop_;
        uv_stream_t                                         *server_;
        uv_tcp_t                                            *client_;
        HandlerType                                         fn;
        std::shared_ptr<Coroutine<void>>                    coroutine;
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
