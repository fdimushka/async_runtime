#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#include "catch.hpp"
#include "ar/ar.hpp"
#include "ar/dataflow/notifier.hpp"
#include "ar/dataflow/kernel_events.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::Dataflow;

enum class FlagValue
{
    Flag1 = 1 << 0, // 1
    Flag2 = 1 << 1, // 2
    Flag3 = 1 << 2, // 4
    Flag4 = 1 << 3, // 8
    Flag5 = 1 << 4, // 16
    Flag6 = 1 << 5, // 32
    Flag7 = 1 << 6, // 64
    Flag8 = 1 << 7  //128
};

FlagValue wait_any(Dataflow::Notifier & notifier) {
    return (FlagValue)Await(notifier.AsyncWatchAny());
}

FlagValue wait_flag1(Dataflow::Notifier & notifier) {
    return (FlagValue)Await(notifier.AsyncWatch((int)FlagValue::Flag1));
}

FlagValue wait_flag2_flag_3(Dataflow::Notifier & notifier) {
    return (FlagValue)Await(notifier.AsyncWatch((int)FlagValue::Flag2 | (int)FlagValue::Flag3));
}

TEST_CASE( "wait notifications", "[notifier]" ) {
    SetupRuntime();
    Dataflow::Notifier notifier;
    notifier.Notify((int)FlagValue::Flag1);
    REQUIRE(wait_flag1(notifier) == FlagValue::Flag1);

    notifier.Notify((int)FlagValue::Flag2);
    notifier.Notify((int)FlagValue::Flag3);

    FlagValue state = wait_flag2_flag_3(notifier);
    REQUIRE( (((int)state & (int)FlagValue::Flag1) != (int)FlagValue::Flag1));
    REQUIRE( (((int)state & (int)FlagValue::Flag2) == (int)FlagValue::Flag2));
    REQUIRE( (((int)state & (int)FlagValue::Flag3) == (int)FlagValue::Flag3));
    Terminate();
}

TEST_CASE( "wait all notifications", "[notifier]" ) {
    SetupRuntime();
    Dataflow::Notifier notifier;
    notifier.Notify((int)FlagValue::Flag1);
    REQUIRE(wait_any(notifier) == FlagValue::Flag1);

    notifier.Notify((int)FlagValue::Flag2);
    notifier.Notify((int)FlagValue::Flag3);

    FlagValue state = wait_any(notifier);
    REQUIRE( (((int)state & (int)FlagValue::Flag1) != (int)FlagValue::Flag1));
    REQUIRE( (((int)state & (int)FlagValue::Flag2) == (int)FlagValue::Flag2));
    Terminate();
}

TEST_CASE( "async wait notifications", "[notifier]" ) {
    SetupRuntime();
    Dataflow::Notifier notifier;
    auto coroutine = make_coroutine<FlagValue>([&notifier](CoroutineHandler *handler, yield<FlagValue> & yield){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return (FlagValue)Await(notifier.AsyncWatch((int)FlagValue::Flag2), handler);
    });

    auto future = Async(coroutine);
    notifier.Notify((int)FlagValue::Flag2);
    future.wait();
    int result = (int)future.get();
    REQUIRE( ((result & (int)FlagValue::Flag1) != (int)FlagValue::Flag1));
    REQUIRE( ((result & (int)FlagValue::Flag2) == (int)FlagValue::Flag2));
    Terminate();
}

TEST_CASE( "async wait any notifications", "[notifier]" ) {
    SetupRuntime();
    Dataflow::Notifier notifier;
    notifier.Notify((int)FlagValue::Flag1);
    REQUIRE(wait_flag1(notifier) == FlagValue::Flag1);
    auto coroutine = make_coroutine<FlagValue>([&notifier](CoroutineHandler *handler, yield<FlagValue> & yield){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return (FlagValue)Await(notifier.AsyncWatchAny(), handler);
    });

    auto future = Async(coroutine);
    notifier.Notify((int)FlagValue::Flag2);
    notifier.Notify((int)FlagValue::Flag3);
    future.wait();
    int result = (int)future.get();
    REQUIRE( ((result & (int)FlagValue::Flag1) != (int)FlagValue::Flag1));
    REQUIRE( ((result & (int)FlagValue::Flag2) == (int)FlagValue::Flag2));
    REQUIRE( ((result & (int)FlagValue::Flag3) == (int)FlagValue::Flag3));
    Terminate();
}

TEST_CASE( "async wait notifications in loop", "[notifier]" ) {
    SetupRuntime();
    Dataflow::Notifier notifier;
    auto coroutine = make_coroutine<FlagValue>([&notifier](CoroutineHandler *handler, yield<FlagValue> & yield) {
        for(;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            int value = Await(notifier.AsyncWatch(FlagValue::Flag2, FlagValue::Flag3, FlagValue::Flag4), handler);
            if ( Dataflow::Notifier::HasState(value, FlagValue::Flag3) ) {
                return (FlagValue) value;
            }
        }
        return (FlagValue)0;
    });

    auto future = Async(coroutine);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    notifier.Notify((int)FlagValue::Flag6);
    notifier.Notify((int)FlagValue::Flag5);
    notifier.Notify((int)FlagValue::Flag3);
    future.wait();
    int result = (int)future.get();
    REQUIRE( ((result & (int)FlagValue::Flag3) == (int)FlagValue::Flag3));
    Terminate();
}



