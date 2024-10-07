#include "ar/ticker.hpp"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;

class ticker_task : public task {
    typedef promise_t<bool> promise_type;
public:
    ticker_task(Timespan &ts) : tick_ts(ts) {};
    ~ticker_task() override = default;

    void execute(const execution_state & state) override {
        try {
            task::state = state;
            tick_ts = TIMESTAMP_NOW_MICRO();
            promise.set_value(true);
        } catch (std::exception & ex) {
            std::cerr << ex.what() << std::endl;
            promise.set_value(false);
        }
    }

    future_t<bool> get_future() { return promise.get_future(); }
private:
    Timespan &tick_ts;
    promise_type promise;
};


void Ticker::Stop() {
    is_continue.store(false, std::memory_order_relaxed);
    //if(tick_result)
    //    tick_result->SetValue(false);
}


future_t<bool> Ticker::AsyncTick(const task::execution_state& execution_state) {
    bool cont = is_continue.load(std::memory_order_relaxed);
    if(!cont) {
        last_tick_ts = TIMESTAMP_NOW_MICRO();
        return make_resolved_future(false);
    }

    Timespan curr_tick_delay = TIMESTAMP_NOW_MICRO() - last_tick_ts;

    if(curr_tick_delay < delay) {
        Runtime::g_runtime->CheckRuntime();
        curr_tick_delay = delay - curr_tick_delay;
        auto task = new ticker_task(last_tick_ts);
        auto future = task->get_future();
        task->set_delay< Timestamp::Micro >(curr_tick_delay);
        task->set_execution_state(execution_state);
        Runtime::g_runtime->Post(task);
        return future;
    } else {
        last_tick_ts = TIMESTAMP_NOW_MICRO();
        return make_resolved_future(true);
    }
}


future_t<bool> Ticker::AsyncTick(CoroutineHandler* handler) {
    return AsyncTick(handler->get_execution_state());
}


future_t<bool> Ticker::AsyncTick() {
    task::execution_state state;
    return AsyncTick(state);
}



