#include <iostream>
#include "ar/ar.hpp"

namespace AR = AsyncRuntime;


void fib(AR::CoroutineHandler* handler, AR::Yield<int> & yield, int count) {
    int first = 1, second = 1;
    yield( first);
    yield( second);
    for ( int i = 0; i < count; ++i) {
        int third = first + second;
        first = second;
        second = third;
        yield( third);
    }
}


int main() {
    AR::Coroutine<int> coro_fib = AR::MakeCoroutine<int>(&fib, 8);
    std::cout << "Fibonacci number: \n";

    for(int x : coro_fib) {
        std::cout << x << std::endl;
    }
    return 0;
}

