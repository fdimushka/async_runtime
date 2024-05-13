#ifndef AR_TICKER_H
#define AR_TICKER_H


#include <iostream>
#include "ar/task.hpp"
#include "ar/timestamp.hpp"
#include "ar/coroutine.hpp"


namespace AsyncRuntime {


    /**
     * @brief
     * @class Ticker
     */
    class Ticker {
    public:
        template< typename Rep, typename Period >
        explicit Ticker(const std::chrono::duration<Rep, Period>& rtime, ObjectID wg = 0):
            delay( Timestamp::Cast<std::chrono::duration<Rep, Period>, Timestamp::Micro>(rtime.count()) )
            , last_tick_ts(TIMESTAMP_NOW_MICRO())
            , is_continue {true}
            { };
        ~Ticker() = default;

        void Stop();

        future_t<bool> AsyncTick();
        future_t<bool> AsyncTick(const task::execution_state& execution_state);
        future_t<bool> AsyncTick(CoroutineHandler* handler);
    private:
        Timespan                        last_tick_ts;
        Timespan                        delay;
        //std::shared_ptr<Result<bool>>   tick_result;
        std::atomic_bool                is_continue;
    };
}

#endif //AR_TICKER_H
