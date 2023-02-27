#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
//#include "../src/Task.h"
//
//
//using namespace AsyncRuntime;
//using namespace std;
//
//
//int foo(int i = 100);
//
//
//int foo(int i) {
//    return i;
//}
//
//
//template<class Callable>
//int run_task(Callable &&f) {
//    auto task = MakeTask(std::forward<Callable>(f));
//    auto result = task->GetResult();
//    {
//        task->Execute();
//        delete task;
//    }
//
//    result->Wait();
//    return result->Get();
//}
//
//
//struct RuntimeException : public exception {
//    const char * what () const throw () {
//        return "Boooo!!!";
//    }
//};
//
//
//struct Foo {
//    int i = 0;
//};
//
//
//TEST_CASE( "Make task test", "[task]" ) {
//    REQUIRE(run_task(bind(&foo, 100)) == 100 );
//    REQUIRE(run_task(bind(bind(&foo, 100))) == 100 );
//    REQUIRE(run_task(bind([](int i){ return i; }, 100)) == 100 );
//    REQUIRE(run_task(bind([](Foo f){ return f.i; }, Foo{100})) == 100 );
//    REQUIRE(run_task(bind([](){ return 0; })) == 0 );
//}
//
//
//TEST_CASE( "Task exception test", "[task]" ) {
//
//    auto task = MakeTask([](int e){
//        throw RuntimeException();
//    });
//
//    auto result = task->GetResult();
//    {
//        task->Execute();
//        delete task;
//    }
//
//
//    try {
//        result->Wait();
//        result->Get();
//    }catch (RuntimeException & e) {
//        REQUIRE(true);
//    }catch (...) {
//        REQUIRE(false);
//    }
//}
//
//
//TEST_CASE( "Task executor test", "[task]" ) {
//    try {
//        auto task = MakeTask([](int e){
//            throw RuntimeException();
//        });
//
//        REQUIRE(task->GetDesirableExecutor() == default_executor);
//        task->SetDesirableExecutor(15);
//        REQUIRE(task->GetDesirableExecutor() == 15);
//
//        auto result = task->GetResult();
//        {
//            task->Execute(10);
//            delete task;
//        }
//
//        result->Wait();
//        result->Get();
//    }catch (RuntimeException & e) {
//        REQUIRE(true);
//    }catch (...) {
//        REQUIRE(false);
//    }
//}
//
//
//TEST_CASE( "Future already retrieved test", "[task]" ) {
//    try {
//        auto task = MakeTask([](int e){
//
//        });
//
//        auto r = task->GetResult();
//        auto r2 = task->GetResult();
//
//        {
//            task->Execute();
//            delete task;
//        }
//
//        r->Wait();
//        r->Get();
//    }catch (const std::future_error& e) {
//        REQUIRE(true);
//    }catch (...) {
//        REQUIRE(false);
//    }
//}
//
//
//TEST_CASE( "Task async test", "[task]" ) {
//    auto task = MakeTask([](int e){
//        return 100;
//    });
//
//    auto r = task->GetResult();
//
//    std::thread([task](){
//        sleep(1);
//        task->Execute();
//        delete task;
//    }).detach();
//
//    r->Wait();
//    REQUIRE(r->Get() == 100);
//}
//
//
//TEST_CASE( "Task async test 2", "[task]" ) {
//    auto task = MakeTask([](int e){
//        return 100;
//    });
//
//    auto task2 = MakeTask([](int e){
//        return 100;
//    });
//
//    auto r = task->GetResult();
//
//    std::thread([task, task2](){
//        auto r2 = task2->GetResult();
//        std::thread([task2]() {
//            sleep(1);
//            task2->Execute();
//            delete task2;
//        }).detach();
//
//        r2->Wait();
//
//        task->Execute();
//        delete task;
//    }).detach();
//
//    r->Wait();
//    REQUIRE(r->Get() == 100);
//}
//
//
//SCENARIO( "Result" ) {
//    GIVEN("An empty result") {
//        Result<void> r;
//
//        THEN("empty result") {
//        }
//    }
//
//
//    GIVEN("wait result") {
//        Result<int> result;
//
//        THEN("wait") {
//            result.SetValue(100);
//            result.Wait();
//            REQUIRE(result.Valid());
//            REQUIRE(result.Get() == 100);
//            REQUIRE(!result.Valid());
//        }
//
//        THEN("no wait") {
//            result.SetValue(100);
//            REQUIRE(result.Valid());
//            REQUIRE(result.Get() == 100);
//            REQUIRE(!result.Valid());
//        }
//
//        THEN("get exception") {
//            REQUIRE(result.Valid());
//            result.SetValue(100);
//            REQUIRE(result.Get() == 100);
//            CHECK_THROWS(result.Get());
//            REQUIRE(!result.Valid());
//        }
//    }
//}

