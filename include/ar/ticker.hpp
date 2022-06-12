#ifndef AR_TICKER_H
#define AR_TICKER_H


#include <iostream>
#include "ar/task.hpp"
#include "ar/helper.hpp"
#include "ar/timestamp.hpp"


namespace AsyncRuntime {


    /**
     * @brief
     * @class Ticker
     */
    class Ticker {
    public:
        template< typename Rep, typename Period >
        explicit Ticker(const std::chrono::duration<Rep, Period>& rtime):
            delay( Timestamp::Cast<std::chrono::duration<Rep, Period>, Timestamp::Micro>(rtime.count()) )
            , last_tick_ts(TIMESTAMP_NOW_MICRO()) { };
        ~Ticker() = default;


        /**
         * @brief
         */
        void Stop();


        /**
         * @brief
         * @return
         */
        std::shared_ptr<Result<bool >> AsyncTick();
    private:
        Timespan last_tick_ts;
        Timespan delay;
    };
}

#endif //AR_TICKER_H
