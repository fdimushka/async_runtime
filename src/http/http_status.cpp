#include <ar/http.hpp>


using namespace AsyncRuntime;
using namespace std;



HTTPStatus int_to_http_status(unsigned v)
{
    switch(static_cast<HTTPStatus>(v))
    {
        // 1xx
        case HTTPStatus::CONTINUE_:
        case HTTPStatus::SWITCHING_PROTOCOLS:
        case HTTPStatus::PROCESSING:
            //[[clang::faLLTHROUGH]];

            // 2xx
        case HTTPStatus::OK:
        case HTTPStatus::CREATED:
        case HTTPStatus::ACCEPTED:
        case HTTPStatus::NON_AUTHORITATIVE_INFORMATION:
        case HTTPStatus::NO_CONTENT:
        case HTTPStatus::RESET_CONTENT:
        case HTTPStatus::PARTIAL_CONTENT:
        case HTTPStatus::MULTI_STATUS:
        case HTTPStatus::ALREADY_REPORTED:
        case HTTPStatus::IM_USED:
            //[[clang::faLLTHROUGH]];;

            // 3xx
        case HTTPStatus::MULTIPLE_CHOICES:
        case HTTPStatus::MOVED_PERMANENTLY:
        case HTTPStatus::FOUND:
        case HTTPStatus::SEE_OTHER:
        case HTTPStatus::NOT_MODIFIED:
        case HTTPStatus::USE_PROXY:
        case HTTPStatus::TEMPORARY_REDIRECT:
        case HTTPStatus::PERMANENT_REDIRECT:
            //[[clang::faLLTHROUGH]];;

            // 4xx
        case HTTPStatus::BAD_REQUEST:
        case HTTPStatus::UNAUTHORIZED:
        case HTTPStatus::PAYMENT_REQUIRED:
        case HTTPStatus::FORBIDDEN:
        case HTTPStatus::NOT_FOUND:
        case HTTPStatus::METHOD_NOT_ALLOWED:
        case HTTPStatus::NOT_ACCEPTABLE:
        case HTTPStatus::PROXY_AUTHENTICATION_REQUIRED:
        case HTTPStatus::REQUEST_TIMEOUT:
        case HTTPStatus::CONFLICT:
        case HTTPStatus::GONE:
        case HTTPStatus::LENGTH_REQUIRED:
        case HTTPStatus::PRECONDITION_FAILED:
        case HTTPStatus::PAYLOAD_TOO_LARGE:
        case HTTPStatus::URI_TOO_LONG:
        case HTTPStatus::UNSUPPORTED_MEDIA_TYPE:
        case HTTPStatus::RANGE_NOT_SATISFIABLE:
        case HTTPStatus::EXPECTATION_FAILED:
        case HTTPStatus::MISDIRECTED_REQUEST:
        case HTTPStatus::UNPROCESSABLE_ENTITY:
        case HTTPStatus::LOCKED:
        case HTTPStatus::FAILED_DEPENDENCY:
        case HTTPStatus::UPGRADE_REQUIRED:
        case HTTPStatus::PRECONDITION_REQUIRED:
        case HTTPStatus::TOO_MANY_REQUESTS:
        case HTTPStatus::REQUEST_HEADER_FIELDS_TOO_LARGE:
        case HTTPStatus::CONNECTION_CLOSED_WITHOUT_RESPONSE:
        case HTTPStatus::UNAVAILABLE_FOR_LEGAL_REASONS:
        case HTTPStatus::CLIENT_CLOSED_REQUEST:
            //[[clang::faLLTHROUGH]];;

            // 5xx
        case HTTPStatus::INTERNAL_SERVER_ERROR:
        case HTTPStatus::NOT_IMPLEMENTED:
        case HTTPStatus::BAD_GATEWAY:
        case HTTPStatus::SERVICE_UNAVAILABLE:
        case HTTPStatus::GATEWAY_TIMEOUT:
        case HTTPStatus::HTTP_VERSION_NOT_SUPPORTED:
        case HTTPStatus::VARIANT_ALSO_NEGOTIATES:
        case HTTPStatus::INSUFFICIENT_STORAGE:
        case HTTPStatus::LOOP_DETECTED:
        case HTTPStatus::NOT_EXTENDED:
        case HTTPStatus::NETWORK_AUTHENTICATION_REQUIRED:
        case HTTPStatus::NETWORK_CONNECT_TIMEOUT_ERROR:
            return static_cast<HTTPStatus>(v);

        default:
            break;
    }
    return HTTPStatus::UNKNOWN;
}


string_view obsolete_reason(HTTPStatus v)
{
    switch(static_cast<HTTPStatus>(v))
    {
        // 1xx
        case HTTPStatus::CONTINUE_:                             return "Continue";
        case HTTPStatus::SWITCHING_PROTOCOLS:                   return "Switching Protocols";
        case HTTPStatus::PROCESSING:                            return "Processing";

            // 2xx
        case HTTPStatus::OK:                                    return "OK";
        case HTTPStatus::CREATED:                               return "Created";
        case HTTPStatus::ACCEPTED:                              return "Accepted";
        case HTTPStatus::NON_AUTHORITATIVE_INFORMATION:         return "Non-Authoritative Information";
        case HTTPStatus::NO_CONTENT:                            return "No Content";
        case HTTPStatus::RESET_CONTENT:                         return "Reset Content";
        case HTTPStatus::PARTIAL_CONTENT:                       return "Partial Content";
        case HTTPStatus::MULTI_STATUS:                          return "Multi-HTTPStatus";
        case HTTPStatus::ALREADY_REPORTED:                      return "Already Reported";
        case HTTPStatus::IM_USED:                               return "IM Used";

            // 3xx
        case HTTPStatus::MULTIPLE_CHOICES:                      return "Multiple Choices";
        case HTTPStatus::MOVED_PERMANENTLY:                     return "Moved Permanently";
        case HTTPStatus::FOUND:                                 return "Found";
        case HTTPStatus::SEE_OTHER:                             return "See Other";
        case HTTPStatus::NOT_MODIFIED:                          return "Not Modified";
        case HTTPStatus::USE_PROXY:                             return "Use Proxy";
        case HTTPStatus::TEMPORARY_REDIRECT:                    return "Temporary Redirect";
        case HTTPStatus::PERMANENT_REDIRECT:                    return "Permanent Redirect";

            // 4xx
        case HTTPStatus::BAD_REQUEST:                           return "Bad Request";
        case HTTPStatus::UNAUTHORIZED:                          return "Unauthorized";
        case HTTPStatus::PAYMENT_REQUIRED:                      return "Payment Required";
        case HTTPStatus::FORBIDDEN:                             return "Forbidden";
        case HTTPStatus::NOT_FOUND:                             return "Not Found";
        case HTTPStatus::METHOD_NOT_ALLOWED:                    return "Method Not Allowed";
        case HTTPStatus::NOT_ACCEPTABLE:                        return "Not Acceptable";
        case HTTPStatus::PROXY_AUTHENTICATION_REQUIRED:         return "Proxy Authentication Required";
        case HTTPStatus::REQUEST_TIMEOUT:                       return "Request Timeout";
        case HTTPStatus::CONFLICT:                              return "Conflict";
        case HTTPStatus::GONE:                                  return "Gone";
        case HTTPStatus::LENGTH_REQUIRED:                       return "Length Required";
        case HTTPStatus::PRECONDITION_FAILED:                   return "Precondition Failed";
        case HTTPStatus::PAYLOAD_TOO_LARGE:                     return "Payload Too Large";
        case HTTPStatus::URI_TOO_LONG:                          return "URI Too Long";
        case HTTPStatus::UNSUPPORTED_MEDIA_TYPE:                return "Unsupported Media Type";
        case HTTPStatus::RANGE_NOT_SATISFIABLE:                 return "Range Not Satisfiable";
        case HTTPStatus::EXPECTATION_FAILED:                    return "Expectation Failed";
        case HTTPStatus::MISDIRECTED_REQUEST:                   return "Misdirected Request";
        case HTTPStatus::UNPROCESSABLE_ENTITY:                  return "Unprocessable Entity";
        case HTTPStatus::LOCKED:                                return "Locked";
        case HTTPStatus::FAILED_DEPENDENCY:                     return "Failed Dependency";
        case HTTPStatus::UPGRADE_REQUIRED:                      return "Upgrade Required";
        case HTTPStatus::PRECONDITION_REQUIRED:                 return "Precondition Required";
        case HTTPStatus::TOO_MANY_REQUESTS:                     return "Too Many Requests";
        case HTTPStatus::REQUEST_HEADER_FIELDS_TOO_LARGE:       return "Request Header Fields Too Large";
        case HTTPStatus::CONNECTION_CLOSED_WITHOUT_RESPONSE:    return "Connection Closed Without Response";
        case HTTPStatus::UNAVAILABLE_FOR_LEGAL_REASONS:         return "Unavailable For Legal Reasons";
        case HTTPStatus::CLIENT_CLOSED_REQUEST:                 return "Client Closed Request";
            // 5xx
        case HTTPStatus::INTERNAL_SERVER_ERROR:                 return "Internal Server Error";
        case HTTPStatus::NOT_IMPLEMENTED:                       return "Not Implemented";
        case HTTPStatus::BAD_GATEWAY:                           return "Bad Gateway";
        case HTTPStatus::SERVICE_UNAVAILABLE:                   return "Service Unavailable";
        case HTTPStatus::GATEWAY_TIMEOUT:                       return "Gateway Timeout";
        case HTTPStatus::HTTP_VERSION_NOT_SUPPORTED:            return "HTTP Version Not Supported";
        case HTTPStatus::VARIANT_ALSO_NEGOTIATES:               return "Variant Also Negotiates";
        case HTTPStatus::INSUFFICIENT_STORAGE:                  return "Insufficient Storage";
        case HTTPStatus::LOOP_DETECTED:                         return "Loop Detected";
        case HTTPStatus::NOT_EXTENDED:                          return "Not Extended";
        case HTTPStatus::NETWORK_AUTHENTICATION_REQUIRED:       return "Network Authentication Required";
        case HTTPStatus::NETWORK_CONNECT_TIMEOUT_ERROR:         return "Network Connect Timeout Error";

        default:
            break;
    }
    return "<unknown-HTTPStatus>";
}

std::ostream& AsyncRuntime::operator << (std::ostream& os, HTTPStatus v)
{
    return os << obsolete_reason(v);
}

