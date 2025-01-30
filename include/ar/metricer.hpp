#ifndef AR_METRICER_H
#define AR_METRICER_H

#include <memory>
#include <string>
#include <map>

namespace AsyncRuntime::Mon {

    class Counter {
    public:
        explicit Counter(const std::string &name, const std::map<std::string, std::string> &labels) {};

        Counter() = default;

        virtual ~Counter() = default;

        virtual void Increment() = 0;

        virtual void Increment(double value) = 0;

        virtual void Decrement() = 0;

        virtual void Decrement(double value) = 0;
    };

    class IMetricer {
    public:
        virtual std::shared_ptr<Counter>
        MakeCounter(const std::string &name, const std::map<std::string, std::string> &labels) = 0;
    };

    template<class CounterT>
    class Metricer : public IMetricer {
        static_assert(std::is_base_of<Counter, CounterT>::value, "CounterT must derive from Counter");
    public:
        explicit Metricer(std::map<std::string, std::string> labels);

        ~Metricer() = default;

        std::shared_ptr<Counter> MakeCounter(const std::string &name, const std::map<std::string, std::string> &labels);

    private:
        std::map<std::string, std::string> const_labels;
    };

    template<class CounterT>
    Metricer<CounterT>::Metricer(std::map<std::string, std::string> labels) : const_labels(std::move(labels)) {}

    template<class CounterT>
    std::shared_ptr<Counter>
    Metricer<CounterT>::MakeCounter(const std::string &name, const std::map<std::string, std::string> &labels) {
        std::map<std::string, std::string> l = const_labels;
        for (const auto &it: labels) {
            l.insert(std::make_pair(it.first, it.second));
        }
        return std::make_shared<CounterT>(name, l);
    }
}

#endif //AR_METRICER_H
