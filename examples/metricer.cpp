#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;

class MyCounter : public Mon::Counter {
public:
    MyCounter(const std::string & name, const std::vector<std::string> &tags) : Mon::Counter(name, tags), name_(name) {};

    void Increment() {
        int v = counter_.fetch_add(1, std::memory_order_relaxed);
        std::cout << name_ << ":" << v + 1 << std::endl;
    }

    void Increment(double value) { }

    void Decrement() {
        int v = counter_.fetch_sub(1, std::memory_order_relaxed);
        std::cout << name_ << ":" << v - 1 << std::endl;
    }

    void Decrement(double value) { }
private:
    std::string name_;
    std::atomic_int counter_ = {0};
};

void async_fun(CoroutineHandler* handler, YieldVoid & yield) {
    yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}


int main() {
    CreateMetricer<MyCounter>({"ar"});
    SetupRuntime();
    for(int i = 0; i < 10; ++i) {
        auto coro = MakeCoroutine(&async_fun);
        Await(Async(coro));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    Terminate();
    return 0;
}

