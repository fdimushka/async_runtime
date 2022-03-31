## Channels
`template<typename T> class Channel`

Channels are the pipes that connect coroutines. You can send values into channels from one coroutines and receive those values into another coroutines.

#### Primary template
The primary Channel template may be instantiated with any TriviallyCopyable type T satisfying both CopyConstructible and CopyAssignable. 
The program is ill-formed if any of following values is false:
* `std::is_trivially_copyable<T>::value`
* `std::is_copy_constructible<T>::value`
* `std::is_move_constructible<T>::value`
* `std::is_copy_assignable<T>::value`
* `std::is_move_assignable<T>::value`

### API
Create channel method `Channel<T> MakeChannel(size_t cap = 64)`:
``` C++
    // T - is type of data.
    Channel<T> channel = MakeChannel<T>();
```

Send data method `bool Send(T && item)`: 
``` C++
    // Sync send method to channel.
    int value = 0;
    channel.Send(value);
```

Receive data method `std::optional<T> Receive()`:
``` C++
    // Sync receive data from channel.
    int result = channel.Receive();
```

Async receive data method `std::shared_ptr<ChannelReceiver<T>> AsyncReceive(Channel<T> *channel)`, 
`ChannelReceiver<T>` this object for awaiting. Use this method only in coroutines.

``` C++
    int res = AR::Await(AsyncReceive(channel), handler);
```

### How to use it
Simple example:
``` C++
namespace AR = AsyncRuntime;
typedef AR::Channel<int>  Channel;


void coro_a_fun(AR::CoroutineHandler* handler, AR::YieldVoid & yield, Channel *channel) {
    yield();
    int i = 0;

    for(;;) {
        channel->Send(i);
        i++;
        yield();
    }
}


void coro_b_fun(AR::CoroutineHandler* handler, AR::YieldVoid & yield, Channel *channel) {
    yield();
    for(;;) {
        int res = AR::Await(AsyncReceive(channel), handler);
        std::cout << "Hello from a: " << res << std::endl;
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
```