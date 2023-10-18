#ifndef AR_METRICER_H
#define AR_METRICER_H

#include <mutex>
#include <memory>
#include <string>
#include <string>
#include <utility>
#include <vector>
#include <map>

namespace AsyncRuntime::Mon {

    class Counter {
    public:
        explicit Counter(const std::string & name, const std::vector<std::string> &tags) { };
        Counter() = default;
        ~Counter() = default;

        virtual void Increment() =0;
        virtual void Increment(double value) =0;

        virtual void Decrement() =0;
        virtual void Decrement(double value) =0;
    };

    class IMetricer {
    public:
        virtual std::shared_ptr< Counter > MakeCounter(const std::string & name, const std::vector<std::string> &tags) =0;
    };

    template< class CounterT >
    class Metricer : public IMetricer {
        static_assert(std::is_base_of<Counter, CounterT>::value, "CounterT must derive from Counter");
    public:
        explicit Metricer(std::vector<std::string> tags);
        ~Metricer() = default;

        std::shared_ptr< Counter > MakeCounter(const std::string & name, const std::vector<std::string> &tags);

    private:
        std::vector<std::string> const_tags;
    };

    template<class CounterT>
    Metricer<CounterT>::Metricer(std::vector<std::string> tags) : const_tags(std::move(tags)) { }

    template< class CounterT >
    std::shared_ptr<Counter> Metricer<CounterT>::MakeCounter(const std::string & name, const std::vector<std::string> &tags) {
        std::vector<std::string> counter_tags = const_tags;
        counter_tags.insert(counter_tags.end(), tags.begin(), tags.end());
        return std::make_shared<CounterT>(name, counter_tags);
    }
}

#endif //AR_METRICER_H
