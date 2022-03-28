#include "ar/ar.hpp"


namespace AR = AsyncRuntime;


void coro_a_fun(AR::YieldVoid & yield, AR::Channel<int>* channel) {
    yield();
    int i = 0;
    for(;;) {
        channel->Send(i);
        i++;
        yield();
    }
}


void coro_b_fun(AR::YieldVoid & yield, AR::Channel<int>* channel) {
    yield();
    for(;;) {
        int res = AR::Await(channel, yield);
        std::cout << res << std::endl;
        yield();
    }
}


int main() {
    AR::SetupRuntime();
    auto channel = AR::MakeChannel<int>();
    AR::Coroutine coro_a = AR::MakeCoroutine(&coro_a_fun, &channel);
    AR::Coroutine coro_b = AR::MakeCoroutine(&coro_b_fun, &channel);

    while (coro_a.Valid() && coro_b.Valid()) {
        AR::Await(AR::Async(coro_a));
        AR::Await(AR::Async(coro_b));
    }

    return 0;
}
