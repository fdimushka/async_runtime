## Channels
`template<typename T> class Channel`

Channels are the pipes that connect coroutines. You can send values into channels from one coroutines and receive those values into another coroutines.

#### Primary template
The primary Channel template may be instantiated with any TriviallyCopyable type T satisfying both CopyConstructible and CopyAssignable. 
The program is ill-formed if any of following values is false:
* `std::is_copy_constructible<T>::value`
* `std::is_move_constructible<T>::value`

### API
Create channel:
``` C++
    // T - is type of data.
    Channel<T> channel;
```

Send data method `void Send(T && item)`: 
``` C++
    // Sync send method to channel.
    int value = 0;
    channel.Send(value);
```

Receive data method `std::optional<T*> Watcher::Receive()`:
``` C++
    // Sync receive data from channel.
    Channel<int> channel;
    ...
    auto watcher = channel.Watch();
    std::optional<int*> result = watcher->Receive();
```

Async receive data method `std::shared_ptr<Result<T*>> Watcher<T>::AsyncReceive()`, 
`ChannelReceiver<T>` this object for awaiting. Use this method only in coroutines.

``` C++
    Channel<int> channel;
    ...
    auto watcher = channel.Watch();
    ...
    auto v = Await(watcher->AsyncReceive());
```

### How to use it
Simple example:
``` C++
#include "ar/ar.hpp"


namespace AR = AsyncRuntime;


[[noreturn]] void async_fun_a(AR::CoroutineHandler* handler, AR::YieldVoid & yield, AR::Channel<std::string> *channel) {
    yield();
    for(;;) {
        channel->Send("ping");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

[[noreturn]] void async_fun_b(AR::CoroutineHandler* handler, AR::YieldVoid & yield, AR::Channel<std::string> *channel) {
    auto watcher = channel->Watch();
    yield();
    for(;;) {
        auto v = AR::Await(watcher->AsyncReceive(), handler);
        std::cout << "recv from channel: " << v->c_str() << std::endl;
        delete v;
    }
}


int main() {
    AR::SetupRuntime();
    AR::Channel<std::string> channel;

    //send a -> b
    AR::Coroutine coro_a = AR::MakeCoroutine(&async_fun_a, &channel);
    AR::Coroutine coro_b = AR::MakeCoroutine(&async_fun_b, &channel);

    const auto& future_a = AR::Async(coro_a);
    const auto& future_b = AR::Async(coro_b);

    AR::Await(future_a);
    AR::Await(future_b);

    return 0;
}
```