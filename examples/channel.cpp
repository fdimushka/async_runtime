#include "ar/ar.hpp"
#include <boost/lockfree/spsc_queue.hpp>

namespace AR = AsyncRuntime;
//
//
//[[noreturn]] void async_fun_a(AR::CoroutineHandler* handler, AR::YieldVoid & yield, AR::Channel<std::string> *channel) {
//    yield();
//    for(;;) {
//        channel->Send("ping");
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }
//}
//
//[[noreturn]] void async_fun_b(AR::CoroutineHandler* handler, AR::YieldVoid & yield, AR::Channel<std::string> *channel) {
//    auto watcher = channel->Watch();
//    yield();
//    for(;;) {
//        auto v = AR::Await(watcher->AsyncReceive(), handler);
//        std::cout << "recv from channel: " << v->c_str() << std::endl;
//        delete v;
//    }
//}

class Foo {
public:
    Foo(int i) : data(i) { };
    ~Foo() {
        std::cout << "~Foo" << std::endl;
    }
    int data = 0;
};

int main() {
    AR::SetupRuntime();

    {
        boost::lockfree::spsc_queue<std::shared_ptr<Foo>> queue(25);
        queue.push(std::make_shared<Foo>(0));
        queue.push(std::make_shared<Foo>(1));
        queue.push(std::make_shared<Foo>(2));
    }
//    AR::Channel<std::string> channel;
//
//    //send a -> b
//    AR::Coroutine coro_a = AR::MakeCoroutine(&async_fun_a, &channel);
//    AR::Coroutine coro_b = AR::MakeCoroutine(&async_fun_b, &channel);
//
//    const auto& future_a = AR::Async(coro_a);
//    const auto& future_b = AR::Async(coro_b);
//
//    AR::Await(future_a);
//    AR::Await(future_b);

    return 0;
}
