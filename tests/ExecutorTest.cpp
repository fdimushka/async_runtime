#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/executor.hpp"
#include "ar/task.hpp"


using namespace AsyncRuntime;
using namespace std::chrono_literals;


//
//class EXECUTOR_TEST_FRIEND {
//public:
//    EXECUTOR_TEST_FRIEND() : executor("") { }
//
//    template< typename Rep, typename Period >
//    ResultVoidPtr Post(DummyTaskImpl *task, const std::chrono::duration<Rep, Period>& rtime) {
//        task->template SetDelay< std::chrono::duration<Rep, Period> >(rtime.count());
//        auto res = task->GetResult();
//        executor.Post(task);
//        return res;
//    }
//
//
//    [[nodiscard]] const Executor::TasksPq& GetPq() const { return executor.delayed_task; }
//
//
//    Executor executor;
//};
//
//
//TEST_CASE( "Executor scheduler test", "[executor]" ) {
//    EXECUTOR_TEST_FRIEND executor_test;
//    auto r1 = executor_test.Post(new DummyTaskImpl(), 5s);
//    auto r2 = executor_test.Post(new DummyTaskImpl(), 1s);
//    auto r3 = executor_test.Post(new DummyTaskImpl(), 3s);
//    auto r4 = executor_test.Post(new DummyTaskImpl(), 4.5s);
//
//
//    REQUIRE(executor_test.GetPq().top()->delay == 1*1000000);
//    r2->Wait();
//
//    REQUIRE(executor_test.GetPq().top()->delay == 3*1000000);
//    r3->Wait();
//
//    auto r5 = executor_test.Post(new DummyTaskImpl(), 0.5s);
//    REQUIRE(executor_test.GetPq().top()->delay == 0.5*1000000);
//    r5->Wait();
//
//    REQUIRE(executor_test.GetPq().top()->delay == 4.5*1000000);
//    r4->Wait();
//
//    REQUIRE(executor_test.GetPq().top()->delay == 5*1000000);
//    r1->Wait();
//}