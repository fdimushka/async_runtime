#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/runtime.hpp"


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
    auto coro1 = make_coroutine(&bar);
    auto coro2 = make_coroutine(&foo, 100);
    auto coro3 = make_coroutine(std::bind(&foo, std::placeholders::_1, std::placeholders::_2, 100));
    auto coro4 = make_coroutine([](CoroutineHandler* handler, YieldVoid &yield, int i){ }, 100);
    auto coro5 = make_coroutine([](CoroutineHandler* handler, YieldVoid &yield){ });
}


TEST_CASE( "Call coroutine test", "[coroutine]" ) {
    bool exec = false;
    auto coro = make_coroutine([&exec](CoroutineHandler* handler, YieldVoid &yield) { exec = true; });
    coro->resume();
    REQUIRE(exec);
}


TEST_CASE( "Coroutine yield test", "[coroutine]" ) {
    int step = 0;

    auto coro = make_coroutine([&step](CoroutineHandler* handler, YieldVoid &yield) {
        step = 1;
        yield();
        step = 2;
        yield();
        step = 3;
    });

    coro->init_promise();
    coro->resume();
    REQUIRE(step == 1);

    coro->init_promise();
    coro->resume();
    REQUIRE(step == 2);

    coro->init_promise();
    coro->resume();

    REQUIRE(step == 3);
    REQUIRE(coro->is_completed());
}


TEST_CASE( "Coroutine call in thread test", "[coroutine]" ) {
    auto coro = make_coroutine<int>([](CoroutineHandler* handler, yield<int> &yield) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        return 100;
    });

    auto task = std::make_shared<coroutine_task<int>>(coro);
    auto f = task->get_future();
    std::thread th([&task](){
        task->execute({});
    });

    REQUIRE(f.get() == 100);
    th.join();
}


TEST_CASE( "Coroutine yield string value test", "[coroutine]" ) {
    auto coro = make_coroutine<std::string>([](CoroutineHandler* handler, yield<std::string> &yield) {
        yield("run");
        yield("hello");
        yield("world");
        return "";
    });

    auto f = coro->get_future();
    coro->resume();
    REQUIRE(f.get() == "run");

    coro->init_promise();
    f = coro->get_future();
    coro->resume();
    REQUIRE(f.get() == "hello");

    coro->init_promise();
    f = coro->get_future();
    coro->resume();
    REQUIRE(f.get() == "world");
}


TEST_CASE( "Generator test", "[coroutine]" ) {
    auto coro = make_coroutine<int>([](CoroutineHandler* handler, yield<int> &yield) {
        for(int i = 0; i < 5; ++i) {
            yield(i);
        }
        return 0;
    });

    int i = 0;

    while (!coro->is_completed()) {
        coro->init_promise();
        auto f = coro->get_future();
        coro->resume();
        i += f.get();
    }

    REQUIRE(i == 10);
}


