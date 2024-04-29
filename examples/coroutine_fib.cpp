#include <iostream>
#include "ar/ar.hpp"

using namespace AsyncRuntime;


int fib(coroutine_handler* handler, yield<int> & yield, int count) {
    int first = 1, second = 1;
    yield( first);
    yield( second);
    for ( int i = 0; i < count; ++i) {
        int third = first + second;
        first = second;
        second = third;
        yield( third);
    }
    return 0;
}


int main() {
    auto coro_fib = make_coroutine<int>(&fib, 8);
    std::cout << "Fibonacci number: \n";

    while (!coro_fib->is_completed()) {
        coro_fib->init_promise();
        auto f = coro_fib->get_future();
        coro_fib->resume();
        std::cout << f.get() << std::endl;
    }
    return 0;
}

