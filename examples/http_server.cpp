//#include "llhttp.h"
//#include "ar/ar.hpp"
//#include <stdio.h>
//#include <string.h>
//
//
//using namespace AsyncRuntime;
//
//int handle_on_message_complete(llhttp_t *llhttp, const char *at, size_t length) {
//    std::cout << std::string(at, length) << std::endl;
//    return 0;
//}
//
//
//void handle_connection(CoroutineHandler *handler, const TCPConnectionPtr& connection) {
//    auto in_stream = MakeStream();
//    int ret = Await(AsyncRead(connection, in_stream), handler);
//    if(ret == IO_SUCCESS) {
//        llhttp_t parser;
//        llhttp_settings_t settings;
//
///* Initialize user callbacks and settings */
//        llhttp_settings_init(&settings);
//        settings.on_url = handle_on_message_complete;
//        llhttp_init(&parser, HTTP_BOTH, &settings);
//        enum llhttp_errno err = llhttp_execute(&parser, in_stream->GetBuffer(), in_stream->GetBufferSize());
//        if (err == HPE_OK) {
//            /* Successfully parsed! */
//            std::string response = "HTTP/1.1 200 OK\n"
//                                   "Server: ar tcp server\n"
//                                   "Accept-Ranges: bytes\n"
//                                   "Content-Length: 12\n"
//                                   "Connection: close\n"
//                                   "Content-Type: text/html\n"
//                                   "\n"
//                                   "Hello world!";
//            auto out_stream = MakeStream(response.c_str(), response.size());
//            ret = Await(AsyncWrite(connection, out_stream), handler);
//            if (ret != IO_SUCCESS) {
//                std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
//            }
//        } else {
//            fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err), parser.reason);
//        }
//    }else{
//        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
//    }
//}


int main() {
//    SetupRuntime();
//    auto server = MakeTCPServer("0.0.0.0", 7000);
//    int ret = Await(AsyncListen(server, &handle_connection));
//    if(ret != IO_SUCCESS) {
//        std::cerr << "error: " << FSErrorMsg(ret) << std::endl;
//    }
//    Terminate();
    return 0;
}