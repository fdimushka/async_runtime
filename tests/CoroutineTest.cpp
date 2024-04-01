#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/coroutine.hpp"


using namespace AsyncRuntime;
using namespace std;


struct Foo {
    int i = 0;
};


void foo(CoroutineHandler* handler, YieldVoid &yield, int i) {
}


void bar(CoroutineHandler* handler, YieldVoid &yield) {
}



TEST_CASE( "Init coroutine test", "[coroutine]" ) {
    auto coro1 = MakeCoroutine(&bar);
    auto coro2 = MakeCoroutine(&foo, 100);
    auto coro3 = MakeCoroutine(std::bind(&foo, std::placeholders::_1, std::placeholders::_2, 100));
    auto coro4 = MakeCoroutine([](CoroutineHandler* handler, YieldVoid &yield, int i){ }, 100);
    auto coro5 = MakeCoroutine([](CoroutineHandler* handler, YieldVoid &yield){ });
}


TEST_CASE( "Call coroutine test", "[coroutine]" ) {
    bool exec = false;
    auto coro = MakeCoroutine([&exec](CoroutineHandler* handler, YieldVoid &yield){ yield(); exec = true; });
    coro();
    REQUIRE(exec);
}


TEST_CASE( "Coroutine yield test", "[coroutine]" ) {
    int step = 0;

    Coroutine coro = MakeCoroutine([&step](CoroutineHandler* handler, YieldVoid &yield) {
        step = 1;
        yield();
        step = 2;
        yield();
        step = 3;
    });

    REQUIRE(step == 1);
    coro();
    REQUIRE(step == 2);
    coro();
    REQUIRE(step == 3);
    REQUIRE(!coro.Valid());

    REQUIRE_THROWS_AS(coro(), std::runtime_error);
}


TEST_CASE( "Coroutine exception test", "[coroutine]" ) {
    auto coro = MakeCoroutine([](CoroutineHandler* handler, YieldVoid &yield) {
        yield();
        throw std::runtime_error("error");
    });

    coro();
    auto result = coro.GetResult();
    REQUIRE_THROWS_AS(result->Get(), std::runtime_error);
}


TEST_CASE( "Coroutine call in thread test", "[coroutine]" ) {
    auto coro = MakeCoroutine<int>([](CoroutineHandler* handler, Yield<int> &yield) {
        yield(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        yield(100);
    });

    coro.MakeResult();
    auto result = coro.GetResult();
    std::thread th([&coro](){
        coro();
    });

    result->Wait();
    REQUIRE(result->Get() == 100);
    th.join();
}


TEST_CASE( "Coroutine create task test", "[coroutine]" ) {
    auto coro = MakeCoroutine<int>([](CoroutineHandler* handler, Yield<int> &yield) {
        yield(0);
        yield(100);
    });

    coro.MakeResult();
    auto *task = coro.MakeExecTask();
    task->Execute(ExecutorState());
    delete task;
    REQUIRE(coro.GetResult()->Get() == 100);
}


TEST_CASE( "Coroutine yield string value test", "[coroutine]" ) {
    auto coro = MakeCoroutine<std::string>([](CoroutineHandler* handler, Yield<std::string> &yield) {
        yield("run");
        yield("hello");
        yield("world");
    });

    coro();
    REQUIRE(coro.GetResult()->Get() == "hello");

    coro();
    REQUIRE(coro.GetResult()->Get() == "world");
}


TEST_CASE( "Generator test", "[coroutine]" ) {
    auto coro = MakeCoroutine<int>([](CoroutineHandler* handler, Yield<int> &yield) {
        yield(0);
        for(int i = 0; i < 5; ++i) {
            yield(i);
        }
    });

    int i = 0;

    while (coro.Valid()) {
        coro();
        if (!coro.Valid()) {
          break;
        }
        i += coro.GetResult()->Get();
    }

    REQUIRE(i == 10);
}


