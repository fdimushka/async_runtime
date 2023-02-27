## How to use it

Include library headers:
``` C++
#include <ar/ar.hpp>
namespace AR = AsyncRuntime;
```

Init default runtime with default setting
``` C++
...
AR::SetupRuntime();
...
```
Basic execution async/await task:
``` C++
int async_func(int param) {
    //computation
    param++;
    return param;
};
...
int res = AR::Await(AR::Async(&async_func, 0)); //await async task
```

Basic execution coroutine:
``` C++
void async_func(AR::YieldVoid & yield, int param) {
    yield(); //suspend this coroutine
    //resume this coroutine
    param++; //computation
    yield(); //suspend this coroutine
    //resume this coroutine
    param = 0;
    //exit coroutine
}

...
AR::Coroutine coro = AR::MakeCoroutine(&async_func, 0); //make coroutine

//execute coroutine
while(coro.Valid()) {
    coro();
}
```

Coroutines and runtime:
```C++
int request(int param) {
    //maybe sync request to external api.
    param++;
    return param;
}


void coro_fun(AR::Yield<int> & yield, int p) {
    for(;;) {
        p = AR::Await(AR::Async(&request, p), yield); //async request and await result
        //big compute operation
        //...
        yield(p); //yielding result big compute operation
    }
}


int main() {
    AR::SetupRuntime();
    AR::Coroutine<int> coro = AR::MakeCoroutine<int>(&coro_fun, 0);

    while (coro.Valid()) {
        int p = AR::Await(AR::Async(coro)); //async execution and await
        std::cout << p << std::endl; //result of big compute operation
    }
    return 0;
}
```

Fibonacci numbers:
``` C++
void fib(AR::Yield<int> & yield, int count) {
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
```

[More examples...](/examples)