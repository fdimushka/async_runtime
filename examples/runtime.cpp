#include "ar/ar.hpp"

namespace AR = AsyncRuntime;


int request(int param) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    param++;
    return param;
}


void async_func(AR::Yield<int> & yield, int p) {
    yield(0);
    for(;;) {
        p = AR::Await(AR::Async(&request, p), yield);
        //big compute operation
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        yield(p);
    }
}


int main() {
    AR::SetupRuntime();
    AR::Coroutine<int> coro = AR::MakeCoroutine<int>(&async_func, 0);

    while (coro.Valid()) {
        int v = AR::Await(AR::Async(coro));
        std::cout << v << std::endl;
    }
    return 0;
}