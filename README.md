## Async runtime library

[![C/C++ CI](https://github.com/fdimushka/async_runtime/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/fdimushka/async_runtime/actions/workflows/build.yml)

AR - it is a C++ library for efficient execution asynchronous tasks, io network operations and parallel heterogeneous computing. 

#### Threads & coroutines & scheduler
Runtime uses efficient work-stealing scheduler to optimize your multithreaded performance and trying to reduce CPU IO wait.
Used asymmetric coroutines based on context switcher implemented in [Boost.Context](https://github.com/boostorg/context).

[Learn more...](docs/arch.md)

## API usage
``` C++
//Include library headers:
#include <ar/ar.hpp
namespace AR = AsyncRuntime;
```

``` C++
//init default runtime with default setting
AR::SetupRuntime();

```
Basic execution async/await task:
``` C++
int async_func(int param) {
    //computation
    param++;
    return param;
};
//...
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

//...
AR::Coroutine coro = AR::MakeCoroutine(&async_func, 0); //make coroutine

//execute coroutine
while(coro.Valid()) {
    coro();
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

Coroutines and async tasks
```C++
int request(int param) {
    //maybe sync request to external api.
    param++;
    return param;
}


void async_func(AR::Yield<int> & yield, int p) {
    for(;;) {
        p = AR::Await(AR::Async(&request, p), yield); //async request and await result
        //big compute operation
        //...
        yield(p); //yielding result big compute operation
    }
}


int main() {
    AR::SetupRuntime();
    AR::Coroutine<int> coro = AR::MakeCoroutine<int>(&async_func, 0);

    while (coro.Valid()) {
        int p = AR::Await(AR::Async(coro)); //async execution and await
        std::cout << p << std::endl; //result of big compute operation
    }
    return 0;
}
```

## Build instructions
To build you will need:

* cmake >= 3.12.4
* gcc >= 4.6
* boost >= 1.70.0

```
mkdir build
cd cmake ..
make 
make install
``` 

CMake option:

| Option            | Description                       | Default |
| :---              |    :----:                         | :----:  |
| `WITH_TRACE`      | build with trace profiler         | OFF     |
| `WITH_TESTS`      | build with unittests              | OFF     |
| `WITH_EXAMPLES`   | build with samples                | ON      |
