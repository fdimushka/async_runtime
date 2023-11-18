#include "ar/http.hpp"
#include "ar/runtime.hpp"
#include "url.hpp"
#include "llhttp.h"


using namespace AsyncRuntime;


#define HTTP_VERSION 11


HTTPResponse::HTTPResponse(HTTPStatus code) : status_code(code), body_size(0)
{
    Set(HTTPField::SERVER, "http server");
    //Set(HTTPField::date, )
}


void HTTPResponse::Set(HTTPField f, int value)
{
    headers.insert(std::make_pair(f, std::to_string(value)));
}


void HTTPResponse::Set(HTTPField f, const std::string &value)
{
    headers.insert(std::make_pair(f,value));
}


void HTTPResponse::Set(const char *data, size_t size)
{
    Set(data, size, "text/html; charset=UTF-8");
}


void HTTPResponse::Set(const char *data, size_t size, const std::string &content_type)
{
    Set(HTTPField::CONTENT_TYPE, content_type);
    Set(HTTPField::CONTENT_LENGTH, static_cast<int>(size));

    body = data;
    body_size = size;
}


const std::string& HTTPResponse::At(HTTPField f)
{
    return headers.at(f);
}


bool HTTPResponse::Exist(HTTPField f)
{
    return headers.find(f) != headers.end();
}


std::ostream &AsyncRuntime::operator<<(std::ostream &os, const HTTPResponse &resp)
{
    unsigned code = static_cast<unsigned>(resp.status_code);
    char buf_[13];
    buf_[0] = 'H';
    buf_[1] = 'T';
    buf_[2] = 'T';
    buf_[3] = 'P';
    buf_[4] = '/';
    buf_[5] = '0' + static_cast<char>(HTTP_VERSION / 10);
    buf_[6] = '.';
    buf_[7] = '0' + static_cast<char>(HTTP_VERSION % 10);
    buf_[8] = ' ';
    buf_[9] = '0' + static_cast<char>(code / 100);
    buf_[10]= '0' + static_cast<char>((code / 10) % 10);
    buf_[11]= '0' + static_cast<char>(code % 10);
    buf_[12]= ' ';

    os.write(&buf_[0], 13);
    os << resp.status_code << "\r\n";

    for(const auto& it : resp.headers)
        os << it.first << ": " << it.second << "\r\n";

    if(resp.body_size > 0) {
        os << "\r\n";
        os.write(resp.body, resp.body_size);
    }

    return os;
}


static int HttpOnHeaderField(llhttp_t* http_t, const char *at, size_t length)
{
    auto *http_request = (HTTPRequest *)http_t->data;
    if(http_request->header_iterator == http_request->headers.end()) {
        http_request->header_iterator = http_request->headers.insert(
                std::make_pair(std::move(std::string(at, length)), "")).first;
    }
    return HPE_OK;
}


static int HttpOnHeaderValue(llhttp_t* http_t, const char *at, size_t length)
{
    auto *http_request = (HTTPRequest *)http_t->data;
    if(http_request->header_iterator != http_request->headers.end()) {
        http_request->header_iterator->second = std::move(std::string(at, length));
        http_request->header_iterator = http_request->headers.end();
    }
    return HPE_OK;
}

static int HttpOnStatus(llhttp_t* http_t, const char *at, size_t length)
{
    auto *http_request = (HTTPRequest *)http_t->data;
    return HPE_OK;
}


static int HttpOnBody(llhttp_t* http_t, const char *at, size_t length)
{
    auto *http_request = (HTTPRequest *)http_t->data;
    http_request->body = at;
    http_request->body_size = length;
    return HPE_OK;
}


static int HttpRequestOnUrl(llhttp_t* http_t, const char *at, size_t length)
{
    auto *http_request = (HTTPRequest *)http_t->data;
    http_request->url = std::move(std::string(at, length));
    return HPE_OK;
}


static int HttpRequestOnUrlComplete(llhttp_t* http_t)
{
    auto *http_request = (HTTPRequest *)http_t->data;
    Url url(http_request->url);
    http_request->path = url.path();
    for(const auto& item : url.query())
        http_request->query.insert(std::make_pair(item.key(), item.val()));

    if(http_t->method >= 0 && http_t->method <= 45) {
        http_request->method = static_cast<HTTPMethod>(http_t->method);
    }else{
        http_request->method = HTTPMethod::UNKNOWN;
    }

    return HPE_OK;
}


static int HttpOnHeaderValueComplete(llhttp_t* http_t)
{
    auto *http_request = (HTTPRequest *)http_t->data;
    http_request->header_iterator = http_request->headers.end();
    return HPE_OK;
}


void HTTPConnection::Init()
{
    std::vector<char> buffer(65536);
    int ret = Await(AsyncRead(coroutine_handler, tcp_connection, buffer.data(), buffer.size()), coroutine_handler);
    if(ret == IO_SUCCESS) {
        llhttp_t parser;
        llhttp_settings_t settings;
        llhttp_settings_init(&settings);

        settings.on_status = &HttpOnStatus;
        settings.on_url = &HttpRequestOnUrl;
        settings.on_body = &HttpOnBody;
        settings.on_url_complete = &HttpRequestOnUrlComplete;
        settings.on_header_field = HttpOnHeaderField;
        settings.on_header_value = HttpOnHeaderValue;
        settings.on_header_value_complete = HttpOnHeaderValueComplete;
        llhttp_init(&parser, HTTP_REQUEST, &settings);

        parser.data = &http_request;
        enum llhttp_errno err = llhttp_execute(&parser, buffer.data(), buffer.size());
        if (err == HPE_OK) {
            valid = true;
        }
    }
}


void HTTPConnection::SetAccessAllowOrigin(const std::string &origin)
{
    access_allow_origin = origin;
}


IOResultPtr
HTTPConnection::AsyncResponse(HTTPResponse &response)
{
    if(!access_allow_origin.empty())
        response.Set(HTTPField::ACCESS_CONTROL_ALLOW_ORIGIN, access_allow_origin);

    std::stringstream ss;
    ss << response;
    std::string str = std::move(ss.str());
    return AsyncWrite(tcp_connection, str.c_str(), str.size());
}


IOResultPtr
HTTPConnection::AsyncResponse(HTTPStatus code, const char *data, size_t size, const std::string &content_type)
{
    HTTPResponse response(code);
    response.Set(data, size, content_type);
    return AsyncResponse(response);
}


IOResultPtr
HTTPConnection::AsyncResponse(HTTPStatus code, const std::string &data, const std::string &content_type)
{
    HTTPResponse response(code);
    response.Set(data.c_str(), data.length(), content_type);
    return AsyncResponse(response);
}


void
HttpServer::AsyncHandleConnection(CoroutineHandler *handler, const TCPConnectionPtr& connection)
{
    auto http_connection = std::make_shared<HTTPConnection>(handler, connection);
    http_connection->Init();
    if(http_connection->Ok()) {
        const auto &request = http_connection->GetRequest();
        auto route_it = routes.find(request.path);
        if (route_it != routes.end()) {
            auto method_it = route_it->second.methods.find(request.method);
            if(method_it != route_it->second.methods.end()) {
                auto &callback = method_it->second;
                callback(handler, http_connection);
            }else {
                HTTPResponse response(HTTPStatus::NOT_FOUND);
                Await(http_connection->AsyncResponse(response), handler);
            }
        } else {
            HTTPResponse response(HTTPStatus::NOT_FOUND);
            Await(http_connection->AsyncResponse(response), handler);
        }
    }
}


void
HttpServer::AddRoute(const std::string &path, HTTPMethod method, const RequestCallbackType& callback)
{
    auto it = routes.find(path);
    if(it == routes.end()) {
        Route route;
        route.methods.insert(std::make_pair(method, callback));
        routes.insert(std::make_pair(path, std::move(route)));
    }else{
        it->second.methods.insert(std::make_pair(method, callback));
    }
}


IOResultPtr
HttpServer::AsyncBind(const std::string &host, int port)
{
    tcp_server = MakeTCPServer(host.c_str(), port);
    return AsyncListen(tcp_server, [this](CoroutineHandler *handler, const TCPConnectionPtr& connection) {
        AsyncHandleConnection(handler, connection);
    });
}


IOResultPtr
HttpServer::AsyncBind(const std::string &host,
                      int port,
                      const std::function<void(void)> &on_bind_success,
                      const std::function<void(int)> &on_bind_error)
{
    tcp_server = MakeTCPServer(host.c_str(), port, on_bind_success, on_bind_error);
    return AsyncListen(tcp_server, [this](CoroutineHandler *handler, const TCPConnectionPtr& connection) {
        AsyncHandleConnection(handler, connection);
    });
}