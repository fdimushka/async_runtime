#ifndef AR_LOGGER_HPP
#define AR_LOGGER_HPP

#ifdef _MSC_VER
#define FUNCTION __FUNCTION__
#define FUNCSIG __FUNCSIG__
#else
#define FUNCTION __PRETTY_FUNCTION__
#define FUNCSIG __PRETTY_FUNCTION__
#endif

#include <mutex>
#include <string>
#include <map>


namespace AsyncRuntime
{
    typedef void (*LoggerCallback_t)(const char * msg, void * user_data);

    class Logger
    {
    public:
        typedef std::map<std::string, std::string> Attrs;

        enum Level
        {
            Silent = 0,
            Error,
            Warning,
            Info,
            Verbose,
            Debug
        };

        enum Mode
        {
            Production = 0,
            E2E
        };

    private:
        LoggerCallback_t _callback;
        void * _user_data;
        mutable std::mutex _mutex;

        Mode  _mode;
        Level _level;
        std::string _domain;
        bool _enable_thread_id;
        bool _enable_timestamp;
        bool _enable_prefix;
        bool _enable_e2e;

    public:
        static Logger s_logger;
        Logger();

        void DuplicateLogger(const Logger& other);

        void SetLogLevel(Level level_);
        void SetModeLevel(Mode level_);
        void SetDomain(std::string domain);
        void Set(LoggerCallback_t callback, void * user_data);
        void Set(LoggerCallback_t callback, void * user_data, Level level);
        //void Get(LoggerCallback_t & callback, void * & user_data); // blame yourself if you have ever thought about using this method!!

        static LoggerCallback_t GetCallback()
        {
            return s_logger._callback;
        }

        void SetEnableThreadId(bool enabled);
        void SetEnableTimestamp(bool enabled);
        void SetEnablePrefix(bool enabled);

        bool GetEnableThreadId() const {return _enable_thread_id;}
        bool GetEnableTimestamp() const {return _enable_timestamp;}
        bool GetEnablePrefix() const {return _enable_prefix;}
        Level GetLevel() const {return _level;}
        Mode GetMode() const { return _mode; }
        std::string GetDomain() const {return _domain;}
        void * GetUserData() const {return _user_data;}

        /**
        * This method is thread-unsafe
        */
        void SetStd(); // for internal use

        void WriteLog(Level level, const std::string & domain, const std::string & message) const;
        void WriteMode(Mode mode, const std::string & domain, const std::string & message) const;
    private:
        void Write(const std::string & tag, const std::string & domain, const std::string & message) const;

        static const char *GetLogPrefix(Level level_);

        static const char *GetModePrefix();

        static void RemoveEscapeSymbols(std::string & str);

#ifdef _MSC_VER
        void DumpMemoryUsage();
#endif
    };

    inline void WriteLog(AsyncRuntime::Logger::Level level, const char domain[], const char message[])
    {
        AsyncRuntime::Logger::s_logger.WriteLog(level, std::string(domain), std::string(message));
    }

#define AR_LOG2(level, msg) \
    AsyncRuntime::Logger::s_logger.WriteLog(AsyncRuntime::Logger::level, std::string(), msg);

#define AR_LOG_SS2(level, msg) \
    { \
    std::stringstream ___ss; \
    ___ss << msg; \
    AsyncRuntime::Logger::s_logger.WriteLog(AsyncRuntime::Logger::level, std::string(), ___ss.str()); \
    }

#define AR_LOG_E2E_SS4(context, status, attrs, msg)  \
    {                                                   \
    auto s_attrs = common::Logger::ConvertAttrs(attrs);         \
    std::stringstream ___ss;                            \
    ___ss << context << " " << status                   \
          << (s_attrs.empty() ? "" : " ") << s_attrs    \
          << " " << msg;                                \
    AsyncRuntime::Logger::s_logger.WriteMode(AsyncRuntime::Logger::E2E, std::string(), ___ss.str()); \
    }

#define AR_LOG3(level, domain, msg) \
    AsyncRuntime::Logger::s_logger.WriteLog(AsyncRuntime::Logger::level, domain, msg);

#define AR_LOG_SS3(level, domain, msg) \
    { \
    std::stringstream ___ss; \
    ___ss << msg; \
    AsyncRuntime::Logger::s_logger.WriteLog(AsyncRuntime::Logger::level, domain, ___ss.str()); \
    }

#define AR_LOG_E2E_SS5(context, status, attrs, domain, msg)  \
    {                                                   \
    auto s_attrs = AsyncRuntime::Logger::ConvertAttrs(attrs);         \
    std::stringstream ___ss;                            \
    ___ss << context << " " << status                   \
          << (s_attrs.empty() ? "" : " ") << s_attrs    \
          << " " << msg;                                \
    AsyncRuntime::Logger::s_logger.WriteMode(AsyncRuntime::Logger::E2E, domain, ___ss.str()); \
    }

#define GET_MACRO2_3(_1,_2,_3,NAME,...) NAME
#define GET_MACRO4_5(_1,_2,_3,_4,_5,NAME,...) NAME

#define AR_LOG(...) GET_MACRO2_3(__VA_ARGS__, AR_LOG3, AR_LOG2)(__VA_ARGS__)
#define AR_LOG_SS(...) GET_MACRO2_3(__VA_ARGS__, AR_LOG_SS3, AR_LOG_SS2)(__VA_ARGS__)
#define AR_CONTEXT  __FILE__ << "(" << __LINE__ << ") : '" << FUNCTION << "': "

#define AR_LOG_CONTEXT(level, msg) \
    AR_LOG_SS(level, AR_CONTEXT << msg);


    struct FunScope
    {
        FunScope(const char * ctx_);
        ~FunScope();
        const char * _ctx;
    };

    // It should be written in the beginning of a function's body. Its level is Verbose.
#define AR_LOG_FUNC \
    AsyncRuntime::FunScope __func_scope__( FUNCTION );
}


#endif //AR_LOGGER_HPP
