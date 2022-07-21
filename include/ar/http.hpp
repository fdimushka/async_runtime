#ifndef AR_HTTP_H
#define AR_HTTP_H

#include <utility>

#include "ar/task.hpp"
#include "ar/helper.hpp"
#include "ar/timestamp.hpp"
#include "ar/net.hpp"


namespace AsyncRuntime {
static  const std::string default_content_type = "text/html; charset=UTF-8";


    enum HTTPMethod {
        DELETE = 0,
        GET = 1,
        HEAD = 2,
        POST = 3,
        PUT = 4,
        CONNECT = 5,
        OPTIONS = 6,
        TRACE = 7,
        COPY = 8,
        LOCK = 9,
        MKCOL = 10,
        MOVE = 11,
        PROPFIND = 12,
        PROPPATCH = 13,
        SEARCH = 14,
        UNLOCK = 15,
        BIND = 16,
        REBIND = 17,
        UNBIND = 18,
        ACL = 19,
        REPORT = 20,
        MKACTIVITY = 21,
        CHECKOUT = 22,
        MERGE = 23,
        MSEARCH = 24,
        NOTIFY = 25,
        SUBSCRIBE = 26,
        UNSUBSCRIBE = 27,
        PATCH = 28,
        PURGE = 29,
        MKCALENDAR = 30,
        LINK = 31,
        UNLINK = 32,
        SOURCE = 33,
        PRI = 34,
        DESCRIBE = 35,
        ANNOUNCE = 36,
        SETUP = 37,
        PLAY = 38,
        PAUSE = 39,
        TEARDOWN = 40,
        GET_PARAMETER = 41,
        SET_PARAMETER = 42,
        REDIRECT = 43,
        RECORD = 44,
        FLUSH = 45,
        UNKNOWN = 46
    };


    enum class HTTPField : unsigned short
    {
        UNKNOWN = 0,
        A_IM,
        ACCEPT,
        ACCEPT_ADDITIONS,
        ACCEPT_CHARSET,
        ACCEPT_DATETIME,
        ACCEPT_ENCODING,
        ACCEPT_FEATURES,
        ACCEPT_LANGUAGE,
        ACCEPT_PATCH,
        ACCEPT_POST,
        ACCEPT_RANGES,
        ACCESS_CONTROL,
        ACCESS_CONTROL_ALLOW_CREDENTIALS,
        ACCESS_CONTROL_ALLOW_HEADERS,
        ACCESS_CONTROL_ALLOW_METHODS,
        ACCESS_CONTROL_ALLOW_ORIGIN,
        ACCESS_CONTROL_EXPOSE_HEADERS,
        ACCESS_CONTROL_MAX_AGE,
        ACCESS_CONTROL_REQUEST_HEADERS,
        ACCESS_CONTROL_REQUEST_METHOD,
        AGE,
        ALLOW,
        ALPN,
        ALSO_CONTROL,
        ALT_SVC,
        ALT_USED,
        ALTERNATE_RECIPIENT,
        ALTERNATES,
        APPARENTLY_TO,
        APPLY_TO_REDIRECT_REF,
        APPROVED,
        ARCHIVE,
        ARCHIVED_AT,
        ARTICLE_NAMES,
        ARTICLE_UPDATES,
        AUTHENTICATION_CONTROL,
        AUTHENTICATION_INFO,
        AUTHENTICATION_RESULTS,
        AUTHORIZATION,
        AUTO_SUBMITTED,
        AUTOFORWARDED,
        AUTOSUBMITTED,
        BASE,
        BCC,
        BODY,
        C_EXT,
        C_MAN,
        C_OPT,
        C_PEP,
        C_PEP_INFO,
        CACHE_CONTROL,
        CALDAV_TIMEZONES,
        CANCEL_KEY,
        CANCEL_LOCK,
        CC,
        CLOSE,
        COMMENTS,
        COMPLIANCE,
        CONNECTION,
        CONTENT_ALTERNATIVE,
        CONTENT_BASE,
        CONTENT_DESCRIPTION,
        CONTENT_DISPOSITION,
        CONTENT_DURATION,
        CONTENT_ENCODING,
        CONTENT_FEATURES,
        CONTENT_ID,
        CONTENT_IDENTIFIER,
        CONTENT_LANGUAGE,
        CONTENT_LENGTH,
        CONTENT_LOCATION,
        CONTENT_MD5,
        CONTENT_RANGE,
        CONTENT_RETURN,
        CONTENT_SCRIPT_TYPE,
        CONTENT_STYLE_TYPE,
        CONTENT_TRANSFER_ENCODING,
        CONTENT_TYPE,
        CONTENT_VERSION,
        CONTROL,
        CONVERSION,
        CONVERSION_WITH_LOSS,
        COOKIE,
        COOKIE2,
        COST,
        DASL,
        DATE,
        DATE_RECEIVED,
        DAV,
        DEFAULT_STYLE,
        DEFERRED_DELIVERY,
        DELIVERY_DATE,
        DELTA_BASE,
        DEPTH,
        DERIVED_FROM,
        DESTINATION,
        DIFFERENTIAL_ID,
        DIGEST,
        DISCARDED_X400_IPMS_EXTENSIONS,
        DISCARDED_X400_MTS_EXTENSIONS,
        DISCLOSE_RECIPIENTS,
        DISPOSITION_NOTIFICATION_OPTIONS,
        DISPOSITION_NOTIFICATION_TO,
        DISTRIBUTION,
        DKIM_SIGNATURE,
        DL_EXPANSION_HISTORY,
        DOWNGRADED_BCC,
        DOWNGRADED_CC,
        DOWNGRADED_DISPOSITION_NOTIFICATION_TO,
        DOWNGRADED_FINAL_RECIPIENT,
        DOWNGRADED_FROM,
        DOWNGRADED_IN_REPLY_TO,
        DOWNGRADED_MAIL_FROM,
        DOWNGRADED_MESSAGE_ID,
        DOWNGRADED_ORIGINAL_RECIPIENT,
        DOWNGRADED_RCPT_TO,
        DOWNGRADED_REFERENCES,
        DOWNGRADED_REPLY_TO,
        DOWNGRADED_RESENT_BCC,
        DOWNGRADED_RESENT_CC,
        DOWNGRADED_RESENT_FROM,
        DOWNGRADED_RESENT_REPLY_TO,
        DOWNGRADED_RESENT_SENDER,
        DOWNGRADED_RESENT_TO,
        DOWNGRADED_RETURN_PATH,
        DOWNGRADED_SENDER,
        DOWNGRADED_TO,
        EDIINT_FEATURES,
        EESST_VERSION,
        ENCODING,
        ENCRYPTED,
        ERRORS_TO,
        ETAG,
        EXPECT,
        EXPIRES,
        EXPIRY_DATE,
        EXT,
        FOLLOWUP_TO,
        FORWARDED,
        FROM,
        GENERATE_DELIVERY_REPORT,
        GETPROFILE,
        HOBAREG,
        HOST,
        HTTP2_SETTINGS,
        IF_,
        IF_MATCH,
        IF_MODIFIED_SINCE,
        IF_NONE_MATCH,
        IF_RANGE,
        IF_SCHEDULE_TAG_MATCH,
        IF_UNMODIFIED_SINCE,
        IM,
        IMPORTANCE,
        IN_REPLY_TO,
        INCOMPLETE_COPY,
        INJECTION_DATE,
        INJECTION_INFO,
        JABBER_ID,
        KEEP_ALIVE,
        KEYWORDS,
        LABEL,
        LANGUAGE,
        LAST_MODIFIED,
        LATEST_DELIVERY_TIME,
        LINES,
        LINK,
        LIST_ARCHIVE,
        LIST_HELP,
        LIST_ID,
        LIST_OWNER,
        LIST_POST,
        LIST_SUBSCRIBE,
        LIST_UNSUBSCRIBE,
        LIST_UNSUBSCRIBE_POST,
        LOCATION,
        LOCK_TOKEN,
        MAN,
        MAX_FORWARDS,
        MEMENTO_DATETIME,
        MESSAGE_CONTEXT,
        MESSAGE_ID,
        MESSAGE_TYPE,
        METER,
        METHOD_CHECK,
        METHOD_CHECK_EXPIRES,
        MIME_VERSION,
        MMHS_ACP127_MESSAGE_IDENTIFIER,
        MMHS_AUTHORIZING_USERS,
        MMHS_CODRESS_MESSAGE_INDICATOR,
        MMHS_COPY_PRECEDENCE,
        MMHS_EXEMPTED_ADDRESS,
        MMHS_EXTENDED_AUTHORISATION_INFO,
        MMHS_HANDLING_INSTRUCTIONS,
        MMHS_MESSAGE_INSTRUCTIONS,
        MMHS_MESSAGE_TYPE,
        MMHS_ORIGINATOR_PLAD,
        MMHS_ORIGINATOR_REFERENCE,
        MMHS_OTHER_RECIPIENTS_INDICATOR_CC,
        MMHS_OTHER_RECIPIENTS_INDICATOR_TO,
        MMHS_PRIMARY_PRECEDENCE,
        MMHS_SUBJECT_INDICATOR_CODES,
        MT_PRIORITY,
        NEGOTIATE,
        NEWSGROUPS,
        NNTP_POSTING_DATE,
        NNTP_POSTING_HOST,
        NON_COMPLIANCE,
        OBSOLETES,
        OPT,
        OPTIONAL,
        OPTIONAL_WWW_AUTHENTICATE,
        ORDERING_TYPE,
        ORGANIZATION,
        ORIGIN,
        ORIGINAL_ENCODED_INFORMATION_TYPES,
        ORIGINAL_FROM,
        ORIGINAL_MESSAGE_ID,
        ORIGINAL_RECIPIENT,
        ORIGINAL_SENDER,
        ORIGINAL_SUBJECT,
        ORIGINATOR_RETURN_ADDRESS,
        OVERWRITE,
        P3P,
        PATH,
        PEP,
        PEP_INFO,
        PICS_LABEL,
        POSITION,
        POSTING_VERSION,
        PRAGMA,
        PREFER,
        PREFERENCE_APPLIED,
        PREVENT_NONDELIVERY_REPORT,
        PRIORITY,
        PRIVICON,
        PROFILEOBJECT,
        PROTOCOL,
        PROTOCOL_INFO,
        PROTOCOL_QUERY,
        PROTOCOL_REQUEST,
        PROXY_AUTHENTICATE,
        PROXY_AUTHENTICATION_INFO,
        PROXY_AUTHORIZATION,
        PROXY_CONNECTION,
        PROXY_FEATURES,
        PROXY_INSTRUCTION,
        PUBLIC_,
        PUBLIC_KEY_PINS,
        PUBLIC_KEY_PINS_REPORT_ONLY,
        RANGE,
        RECEIVED,
        RECEIVED_SPF,
        REDIRECT_REF,
        REFERENCES,
        REFERER,
        REFERER_ROOT,
        RELAY_VERSION,
        REPLY_BY,
        REPLY_TO,
        REQUIRE_RECIPIENT_VALID_SINCE,
        RESENT_BCC,
        RESENT_CC,
        RESENT_DATE,
        RESENT_FROM,
        RESENT_MESSAGE_ID,
        RESENT_REPLY_TO,
        RESENT_SENDER,
        RESENT_TO,
        RESOLUTION_HINT,
        RESOLVER_LOCATION,
        RETRY_AFTER,
        RETURN_PATH,
        SAFE,
        SCHEDULE_REPLY,
        SCHEDULE_TAG,
        SEC_FETCH_DEST,
        SEC_FETCH_MODE,
        SEC_FETCH_SITE,
        SEC_FETCH_USER,
        SEC_WEBSOCKET_ACCEPT,
        SEC_WEBSOCKET_EXTENSIONS,
        SEC_WEBSOCKET_KEY,
        SEC_WEBSOCKET_PROTOCOL,
        SEC_WEBSOCKET_VERSION,
        SECURITY_SCHEME,
        SEE_ALSO,
        SENDER,
        SENSITIVITY,
        SERVER,
        SET_COOKIE,
        SET_COOKIE2,
        SETPROFILE,
        SIO_LABEL,
        SIO_LABEL_HISTORY,
        SLUG,
        SOAPACTION,
        SOLICITATION,
        STATUS_URI,
        STRICT_TRANSPORT_SECURITY,
        SUBJECT,
        SUBOK,
        SUBST,
        SUMMARY,
        SUPERSEDES,
        SURROGATE_CAPABILITY,
        SURROGATE_CONTROL,
        TCN,
        TE,
        TIMEOUT,
        TITLE,
        TO,
        TOPIC,
        TRAILER,
        TRANSFER_ENCODING,
        TTL,
        UA_COLOR,
        UA_MEDIA,
        UA_PIXELS,
        UA_RESOLUTION,
        UA_WINDOWPIXELS,
        UPGRADE,
        URGENCY,
        URI,
        USER_AGENT,
        VARIANT_VARY,
        VARY,
        VBR_INFO,
        VERSION,
        VIA,
        WANT_DIGEST,
        WARNING,
        WWW_AUTHENTICATE,
        X_ARCHIVED_AT,
        X_DEVICE_ACCEPT,
        X_DEVICE_ACCEPT_CHARSET,
        X_DEVICE_ACCEPT_ENCODING,
        X_DEVICE_ACCEPT_LANGUAGE,
        X_DEVICE_USER_AGENT,
        X_FRAME_OPTIONS,
        X_MITTENTE,
        X_PGP_SIG,
        X_RICEVUTA,
        X_RIFERIMENTO_MESSAGE_ID,
        X_TIPORICEVUTA,
        X_TRASPORTO,
        X_VERIFICASICUREZZA,
        X400_CONTENT_IDENTIFIER,
        X400_CONTENT_RETURN,
        X400_CONTENT_TYPE,
        X400_MTS_IDENTIFIER,
        X400_ORIGINATOR,
        X400_RECEIVED,
        X400_RECIPIENTS,
        X400_TRACE,
        XREF
    };


    std::ostream& operator << (std::ostream& os, HTTPField f);


    enum class HTTPStatus : unsigned
    {
        UNKNOWN = 0,

        CONTINUE_                           = 100,
        SWITCHING_PROTOCOLS                 = 101,

        PROCESSING                          = 102,

        OK                                  = 200,
        CREATED                             = 201,
        ACCEPTED                            = 202,
        NON_AUTHORITATIVE_INFORMATION       = 203,
        NO_CONTENT                          = 204,
        RESET_CONTENT                       = 205,
        PARTIAL_CONTENT                     = 206,
        MULTI_STATUS                        = 207,
        ALREADY_REPORTED                    = 208,
        IM_USED                             = 226,

        MULTIPLE_CHOICES                    = 300,
        MOVED_PERMANENTLY                   = 301,
        FOUND                               = 302,
        SEE_OTHER                           = 303,
        NOT_MODIFIED                        = 304,
        USE_PROXY                           = 305,
        TEMPORARY_REDIRECT                  = 307,
        PERMANENT_REDIRECT                  = 308,

        BAD_REQUEST                         = 400,
        UNAUTHORIZED                        = 401,
        PAYMENT_REQUIRED                    = 402,
        FORBIDDEN                           = 403,
        NOT_FOUND                           = 404,
        METHOD_NOT_ALLOWED                  = 405,
        NOT_ACCEPTABLE                      = 406,
        PROXY_AUTHENTICATION_REQUIRED       = 407,
        REQUEST_TIMEOUT                     = 408,
        CONFLICT                            = 409,
        GONE                                = 410,
        LENGTH_REQUIRED                     = 411,
        PRECONDITION_FAILED                 = 412,
        PAYLOAD_TOO_LARGE                   = 413,
        URI_TOO_LONG                        = 414,
        UNSUPPORTED_MEDIA_TYPE              = 415,
        RANGE_NOT_SATISFIABLE               = 416,
        EXPECTATION_FAILED                  = 417,
        MISDIRECTED_REQUEST                 = 421,
        UNPROCESSABLE_ENTITY                = 422,
        LOCKED                              = 423,
        FAILED_DEPENDENCY                   = 424,
        UPGRADE_REQUIRED                    = 426,
        PRECONDITION_REQUIRED               = 428,
        TOO_MANY_REQUESTS                   = 429,
        REQUEST_HEADER_FIELDS_TOO_LARGE     = 431,
        CONNECTION_CLOSED_WITHOUT_RESPONSE  = 444,
        UNAVAILABLE_FOR_LEGAL_REASONS       = 451,
        CLIENT_CLOSED_REQUEST               = 499,

        INTERNAL_SERVER_ERROR               = 500,
        NOT_IMPLEMENTED                     = 501,
        BAD_GATEWAY                         = 502,
        SERVICE_UNAVAILABLE                 = 503,
        GATEWAY_TIMEOUT                     = 504,
        HTTP_VERSION_NOT_SUPPORTED          = 505,
        VARIANT_ALSO_NEGOTIATES             = 506,
        INSUFFICIENT_STORAGE                = 507,
        LOOP_DETECTED                       = 508,
        NOT_EXTENDED                        = 510,
        NETWORK_AUTHENTICATION_REQUIRED     = 511,
        NETWORK_CONNECT_TIMEOUT_ERROR       = 599
    };


    std::ostream& operator << (std::ostream& os, HTTPStatus v);


    struct HTTPRequest {
        HTTPMethod                                      method = UNKNOWN;
        std::string                                     url;
        std::string                                     path;
        const char*                                     body = nullptr;
        size_t                                          body_size =0;
        std::unordered_map<std::string, std::string>    query;
        std::unordered_map<std::string, std::string>    headers;
        std::unordered_map<std::string, std::string>::iterator header_iterator = headers.end();
    };


    class HTTPResponse {
    public:
        HTTPResponse(HTTPStatus code);

        void Set(HTTPField f, const std::string& value);
        void Set(HTTPField f, int value);
        void Set(const char *data, size_t size, const std::string& content_type);
        void Set(const char *data, size_t size);
        bool Exist(HTTPField f);
        const std::string& At(HTTPField f);

        friend std::ostream &operator<<(std::ostream &os, const HTTPResponse &resp);
    private:
        HTTPStatus                                      status_code;
        std::string                                     url;
        std::unordered_map<HTTPField, std::string>      headers;
        const char*                                     body;
        size_t                                          body_size =0;
    };

    std::ostream &operator<<(std::ostream &os, const HTTPResponse &resp);

    class HttpServer;


    /**
     * @brief HTTP connection class
     * @class HTTPConnection
     */
    class HTTPConnection : public std::enable_shared_from_this<HTTPConnection> {
        friend HttpServer;
        typedef int                                                         IOResult;
        typedef std::shared_ptr<Result<IOResult>>                           IOResultPtr;
    public:
        explicit HTTPConnection(CoroutineHandler *handler, const TCPConnectionPtr&  connection)
        : tcp_connection(connection)
        , coroutine_handler(handler)
        , valid(false)
        , in_stream(MakeStream())
        {
        }


        void SetAccessAllowOrigin(const std::string &origin);


        /**
         * @brief
         * @param response
         * @return
         */
        IOResultPtr AsyncResponse(HTTPResponse &response);
        IOResultPtr AsyncResponse(HTTPStatus code,
                                  const char* data, size_t size,
                                  const std::string& content_type=default_content_type);
        IOResultPtr AsyncResponse(HTTPStatus code,
                                  const std::string& data,
                                  const std::string& content_type=default_content_type);


        bool Ok() const { return valid; }


        const HTTPRequest& GetRequest() const { return http_request;}
    private:
        void Init();

        std::string         access_allow_origin;
        HTTPRequest         http_request;
        TCPConnectionPtr    tcp_connection;
        CoroutineHandler    *coroutine_handler;
        bool                valid;
        IOStreamPtr         in_stream;
    };
    typedef std::shared_ptr<HTTPConnection> HTTPConnectionPtr;


    /**
     * @brief
     * @class HttpServer
     */
    class HttpServer {
    public:
        typedef int                                                         IOResult;
        typedef std::shared_ptr<Result<IOResult>>                           IOResultPtr;
        typedef std::function<void(CoroutineHandler*, HTTPConnectionPtr)>   RequestCallbackType;


        HttpServer() = default;
        ~HttpServer() = default;


        /**
         * @brief
         * @param path
         * @param callback
         */
        void AddRoute(const std::string &path, HTTPMethod method, const RequestCallbackType& callback);

        /**
         * @brief async bind http server
         * @param host
         * @param port
         * @return
         */
        IOResultPtr AsyncBind(const std::string &host, int port);


        /**
         * @brief
         * @param host
         * @param port
         * @param on_bind_success
         * @param on_bind_error
         * @return
         */
        IOResultPtr AsyncBind(const std::string &host,
                              int port,
                              const std::function<void(void)>& on_bind_success,
                              const std::function<void(int)>& on_bind_error);


        /**
         * @brief
         * @param handler
         * @param connection
         */
        void AsyncHandleConnection(CoroutineHandler *handler, const TCPConnectionPtr& connection);
    private:
        struct Route {
            std::map<HTTPMethod, RequestCallbackType> methods;
        };


        TCPServerPtr                                            tcp_server;
        std::unordered_map<std::string, Route>                  routes;
    };
}

#endif //AR_HTTP_H
