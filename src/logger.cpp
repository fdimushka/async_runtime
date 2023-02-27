#include "ar/logger.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _MSC_VER
#include <psapi.h>
#pragma comment (lib, "psapi.lib")
#endif // _MSC_VER

using namespace AsyncRuntime;

AsyncRuntime::Logger AsyncRuntime::Logger::s_logger;

void StdWrite(const char * msg, void *) {
    std::cout << msg << std::flush;
    std::cout.flush();
}

AsyncRuntime::Logger::Logger()
        : _callback(0)
        , _user_data(0)
        , _level(Verbose)
        , _enable_thread_id(false)
        , _enable_timestamp(true)
        , _enable_prefix(true) {
}

void AsyncRuntime::Logger::DuplicateLogger(const Logger& other) {
    std::lock_guard<std::mutex> lock_guard(_mutex);
    _callback = Logger::GetCallback();
    _user_data = other.GetUserData();
    _level = other.GetLevel();
    _enable_thread_id = other.GetEnableThreadId();
    _enable_timestamp = other.GetEnableTimestamp();
    _enable_prefix = other.GetEnablePrefix();
    _domain = other.GetDomain();
}

void AsyncRuntime::Logger::SetLogLevel(Level level_) {
    if (level_ > Debug)
        throw std::runtime_error("invalid logging level");
    _level = level_;
}


void AsyncRuntime::Logger::SetModeLevel(Mode mode_) {
    if (mode_ > E2E)
        throw std::runtime_error("invalid mode level");
    _mode = mode_;
}


void AsyncRuntime::Logger::SetDomain(std::string domain) {
    _domain = domain;
}


void AsyncRuntime::Logger::Set(LoggerCallback_t callback, void * user_data) {
    std::lock_guard<std::mutex> lock(_mutex);
    _user_data = user_data;
    _callback = callback;
}

void AsyncRuntime::Logger::Set(LoggerCallback_t callback, void * user_data, Level level) {
    Set(callback, user_data);
    SetLogLevel(level);
}


void AsyncRuntime::Logger::SetEnableThreadId(bool enabled) {
    _enable_thread_id = enabled;
}


void AsyncRuntime::Logger::SetEnableTimestamp(bool enabled) {
    _enable_timestamp = enabled;
}


void AsyncRuntime::Logger::SetEnablePrefix(bool enabled) {
    _enable_prefix = enabled;
}


void AsyncRuntime::Logger::SetStd()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _callback = StdWrite;
}

void AsyncRuntime::Logger::WriteMode(Mode mode, const std::string & domain, const std::string & message) const
{
    if (_mode != E2E || mode != E2E)
        return;

    Write(GetModePrefix(), domain.empty() ? _domain : domain, message);
}

void AsyncRuntime::Logger::WriteLog(Level level, const std::string & domain, const std::string & message) const
{
    if (_level < level)
        return;

    Write(GetLogPrefix(level), domain.empty() ? _domain : domain, message);
}

void AsyncRuntime::Logger::Write(const std::string & tag, const std::string & domain, const std::string & message) const
{
    if (_callback == 0)
        return;

    std::stringstream ss;

    if (_enable_prefix) {
        ss << "[" << tag << "] ";
    }

    if (!domain.empty()) {
        ss << "[" << domain << "] ";
    }

    if (_enable_timestamp)
    {
        std::time_t time_since_epoch;
        std::time( &time_since_epoch );   // get the time since epoch
        std::tm *tm = std::localtime( &time_since_epoch );  // convert to broken-down local time
        if (tm != 0)
        {
            ss      << "["
                    << std::put_time(tm, "%b %d %T")
                    << "] ";
        }
    }

    if (_enable_thread_id)
    {
        ss << "[" << std::this_thread::get_id() << "] ";
    }


    auto cleaned = message;
    RemoveEscapeSymbols(cleaned);

    ss << cleaned;
    ss << std::endl;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_callback != 0)
            _callback(ss.str().c_str(), _user_data);
    }
}


//static
void AsyncRuntime::Logger::RemoveEscapeSymbols(std::string & str)
{
    std::vector<char> symbols = {'\n', '\r'};
    for (const auto & symbol : symbols)
        str.erase(std::remove(str.begin(), str.end(), symbol), str.end());
}


// static
const char *AsyncRuntime::Logger::GetLogPrefix( Level level_ ) {
    switch (level_)
    {
        case Error :
            return "E";
        case Warning:
            return "W";
        case Info:
            return "I";
        case Verbose:
            return "V";
        case Debug:
            return "D";
        case Silent:
            return "S";
    }
    assert(!"invalid level");
    return "  ";
}


// static
const char *AsyncRuntime::Logger::GetModePrefix() {
    switch (1) {
        case E2E:
            return "E2E";
        case Production:
        default:
            return "  ";
    }
}


#ifdef _MSC_VER
void AsyncRuntime::Logger::DumpMemoryUsage()
{
    static const int BYTES_IN_MEGABYTE = 1024 * 1024;

    HANDLE hProcess = ::GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;

    if (::GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
    {
        std::stringstream ss;
        ss << "PagefileUsage: " << pmc.PagefileUsage / BYTES_IN_MEGABYTE << " Mb" << std::endl;
        ss << "PeakPagefileUsage: " << pmc.PeakPagefileUsage / BYTES_IN_MEGABYTE << " Mb" << std::endl;

        Write(Info, ss.str());
    }
}
#endif

AsyncRuntime::FunScope::FunScope(const char * ctx_) : _ctx(ctx_)
{
    AR_LOG_SS(Logger::Verbose, _ctx << " started...")
}

AsyncRuntime::FunScope::~FunScope()
{
    AR_LOG_SS(Logger::Verbose, _ctx << " finished.")
}

