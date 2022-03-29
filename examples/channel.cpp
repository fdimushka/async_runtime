#include "ar/ar.hpp"


namespace AR = AsyncRuntime;


void coro_a_fun(AR::YieldVoid & yield, AR::Channel<int>* channel) {
    yield();
    int i = 0;
    for(;;) {
        channel->Send(i);
        i++;

        if(i > 1000) {
            break;
        }
        yield();
    }
}


void coro_b_fun(AR::YieldVoid & yield, AR::Channel<int>* channel) {
    yield();
    for(;;) {
        //int res = AR::Await(channel->AsyncReceive(), yield.coroutine_handler);
        //std::cout << res << std::endl;
    }
}


int main() {
    AR::SetupRuntime();
    auto channel = AR::MakeChannel<int>();
    AR::Coroutine coro_a = AR::MakeCoroutine(&coro_a_fun, &channel);
    AR::Coroutine coro_b = AR::MakeCoroutine(&coro_b_fun, &channel);

    AR::Async(coro_b);

    while (coro_a.Valid()) {
        AR::Await(AR::Async(coro_a));
    }

    return 0;
}
