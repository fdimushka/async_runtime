#ifndef AR_TIMESTAMP_H
#define AR_TIMESTAMP_H


#include <chrono>
#include <ctime>


namespace AsyncRuntime {
    typedef int64_t Timespan;

    class Timestamp {
        unsigned long long _time;
    public:
        typedef std::chrono::milliseconds Milli;
        typedef std::chrono::microseconds Micro;

        static inline Timespan Now() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
        }

        static inline Timespan NowMicro() {
            return std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
        }

        static inline Timespan NowSec() {
            return std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
        }


        template<typename From, typename To, typename T>
        static inline Timespan Cast(T time) {
            return std::chrono::duration_cast<To>(From(time)).count();
        }


        template< typename Rep, typename Period >
        static inline Timespan CastMicro(const std::chrono::duration<Rep, Period>& rtime) {
            return Timestamp::Cast<std::chrono::duration<Rep, Period>, Timestamp::Micro>(rtime.count());
        }


        template< typename Rep, typename Period >
        static inline Timespan CastMilli(const std::chrono::duration<Rep, Period>& rtime) {
            return Timestamp::Cast<std::chrono::duration<Rep, Period>, Timestamp::Milli>(rtime.count());
        }
    };

#define TIMESTAMP_NOW()          Timestamp::Now()
#define TIMESTAMP_NOW_MICRO()    Timestamp::NowMicro()
#define TIMESTAMP_NOW_SEC()      Timestamp::NowSec()

}


#endif //AR_TIMESTAMP_H
