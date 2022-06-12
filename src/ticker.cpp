#include "ar/ticker.hpp"
#include "ar/runtime.hpp"

using namespace AsyncRuntime;


class TickerTaskImpl : public Task
{
public:
    explicit TickerTaskImpl(Timespan &ts) : result(new Result<bool>() ), tick_ts(ts){ };
    ~TickerTaskImpl() override = default;


    void Execute(const ExecutorState& executor_) override {
        try {
            executor = executor_;
            tick_ts = TIMESTAMP_NOW_MICRO();
            result->SetValue(true);
        } catch(...) {
            try {
                result->SetException(std::current_exception());
            } catch(...) { }
        }
    }


    [[nodiscard]] std::shared_ptr<Result<bool>> GetResult() const { return result; }
private:
    Timespan &tick_ts;
    std::shared_ptr<Result<bool>>                                   result;
};


void Ticker::Stop()
{

}


std::shared_ptr<Result<bool>> Ticker::AsyncTick()
{
    Timespan curr_tick_delay = TIMESTAMP_NOW_MICRO() - last_tick_ts;

    if(curr_tick_delay < delay) {
        Runtime::g_runtime.CheckRuntime();
        curr_tick_delay = delay - curr_tick_delay;
        auto task = new TickerTaskImpl(last_tick_ts);
        task->template SetDelay< Timestamp::Micro >(curr_tick_delay);
        auto result = task->GetResult();
        Runtime::g_runtime.Post(task);
        return result;
    } else {
        last_tick_ts = TIMESTAMP_NOW_MICRO();
        return std::make_shared<Result<bool>>(true);
    }
}
