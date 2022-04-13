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


    class TCPSession : public std::enable_shared_from_this<TCPSession> {
    public:
        explicit TCPSession(uv_stream_t *server, const std::function<void(std::shared_ptr<TCPSession>)> & connection_handler);
        ~TCPSession();

        void Run();
        void Invoke(CoroutineHandler *handler);


        bool IsAccepted() const { return accepted_; }
        void Accept();


        uv_tcp_t* GetClient() { return client_; }
        const uv_tcp_t* GetClient() const { return client_; }


        uv_stream_t* GetServer() { return server_; }
        const uv_stream_t* GetServer() const { return server_; }


        CoroutineHandler* GetCoroutineHandler() { return coroutine_handler_; };
        const CoroutineHandler* GetCoroutineHandler() const { return coroutine_handler_; };
    private:
        static void Session(CoroutineHandler *handler, YieldVoid yield, const std::shared_ptr<TCPSession>& session) {
            //accept
            yield();

            //resume session & invoke user session
            session->Invoke(handler);
        }


        uv_loop_t                                           *loop_;
        uv_stream_t                                         *server_;
        uv_tcp_t                                            *client_;
        std::function<void(std::shared_ptr<TCPSession>)>    handler;
        std::shared_ptr<Coroutine<void>>                    coroutine;
        CoroutineHandler*                                   coroutine_handler_;
        bool                                                accepted_;
    };


    typedef std::shared_ptr<TCPSession>  TCPSessionPtr;


    /**
     * @brief create tcp server;
     * @param hostname
     * @param port
     * @return
     */
    TCPServerPtr MakeTCPServer(const char* hostname, int port);
}

#endif //AR_SERVER_H
