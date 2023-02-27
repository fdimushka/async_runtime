#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/ar.hpp"


#include <vector>
#include <thread>


using namespace AsyncRuntime;

struct Message {
    int v_int = 0;

    Message() = default;
    ~Message() = default;
    Message(const Message& other) {
        v_int = other.v_int;
    };
};


TEMPLATE_TEST_CASE( "Create channel test", "[channel]", int, std::string, std::vector<int>, Message ) {
    Channel<TestType> channel;
}


TEMPLATE_TEST_CASE( "Send and receive to channel test (type)", "[channel]", int, std::string, std::vector<int>, Message ) {
    Channel<TestType> channel;
    auto watcher = channel.Watch();

    TestType msg;
    channel.Send(msg);
    auto recv_v = watcher->Receive();
    REQUIRE(recv_v);
}


TEST_CASE( "Send and receive to channel test", "[channel]" ) {
    Channel<int> channel;
    auto watcher = channel.Watch();
    int msg = 10;
    channel.Send(msg);
    auto recv_v = watcher->Receive();
    REQUIRE(recv_v);
    int *v_ptr = recv_v.value();
    REQUIRE(*v_ptr == 10);
    delete v_ptr;
}


TEST_CASE( "Try receive from channel test", "[channel]" ) {
    Channel<int> channel;
    int msg = 10;

    std::thread th([=](std::shared_ptr<Watcher<int>> watcher){
        for(;;) {
            auto recv_v = watcher->TryReceive();
            if(recv_v) {
                int &v = *recv_v.value();
                REQUIRE(v == 10);
                break;
            }
        }
    }, channel.Watch());

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    channel.Send(msg);
    th.join();
}


void async_fun(CoroutineHandler* handler, Yield<int> & yield, Channel<int> *channel) {
    auto watcher = channel->Watch();
    yield(0);
    auto v = Await(watcher->AsyncReceive(), handler);
    yield(*v);
}


TEST_CASE( "Async receive from channel test", "[channel]" ) {
    SetupRuntime();
    Channel<int> channel;
    int msg = 10;
    Coroutine<int> coro = MakeCoroutine<int>(&async_fun, &channel);

    channel.Send(msg);

    int v = Await(Async(coro));
    REQUIRE(msg == v);
}


TEST_CASE( "Async receive from channel test 2", "[channel]" ) {
    SetupRuntime();
    Channel<int> channel;
    std::shared_ptr<Watcher<int>> watcher = channel.Watch();


    for(int i = 0; i < 100; ++i) {
        int msg = i;
        channel.Send(msg);
    }

    for(int i = 0; i < 100; ++i) {
        Coroutine<int*> coro = MakeCoroutine<int*>([](CoroutineHandler* handler, Yield<int*> & yield, const std::shared_ptr<Watcher<int>>& watcher){
            yield(nullptr);
            auto v = Await(watcher->AsyncReceive(), handler);
            yield(v);
        }, watcher);
        int *v = Await(Async(coro));
        REQUIRE(i == *v);
        delete v;
    }
}


TEST_CASE( "Watch channel test", "[channel]" ) {
    Channel<int> channel;
    int msg = 10;
    auto watcher1 = channel.Watch();
    auto watcher2 = channel.Watch();

    channel.Send(msg);
    auto v1 = watcher1->Receive();
    auto v2 = watcher2->Receive();

    REQUIRE(v1);
    REQUIRE(v2);
    auto msg1 = v1.value();
    auto msg2 = v2.value();

    REQUIRE(*msg1 == msg);
    REQUIRE(*msg2 == msg);

    delete msg1;
    delete msg2;
}


TEST_CASE( "Watch channel test 2", "[channel]" ) {
    Channel<int> channel;
    int msg = 10;

    {
        auto watcher = channel.Watch();
        channel.Send(msg);
    }
}
