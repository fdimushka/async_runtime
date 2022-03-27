#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
//#include "../src/Notifier.h"
//
//
//using namespace AsyncRuntime;
//
//
//TEST_CASE( "Create notifier test", "[notifier]" ) {
//    Notifier  notifier(std::thread::hardware_concurrency());
//    REQUIRE(notifier.Size() == std::thread::hardware_concurrency());
//
//    Notifier  notifier2(0);
//    REQUIRE(notifier2.Size() == 0);
//}
//
//
//TEST_CASE( "Notify test", "[notifier]" ) {
//    Notifier  notifier(std::thread::hardware_concurrency());
//
//    std::thread th([&notifier](){
//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//        notifier.Notify(false);
//    });
//
//    std::thread th2([&notifier](){
//        notifier.PrepareWait(notifier.GetWaiter(0));
//        notifier.Wait(notifier.GetWaiter(0));
//    });
//
//    th2.join();
//    th.join();
//}
//
//
//TEST_CASE( "Notify signaled test", "[notifier]" ) {
//    Notifier  notifier(std::thread::hardware_concurrency());
//    std::vector<std::thread> threads;
//
//    for(size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
//        threads.emplace_back(std::move(std::thread([&notifier](int index) {
//                notifier.PrepareWait(notifier.GetWaiter(index));
//                notifier.Wait(notifier.GetWaiter(index));
//        }, i)));
//    }
//
//
//    std::thread th([&notifier](){
//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//        notifier.Notify(true);
//    });
//
//    for(size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
//        threads[i].join();
//    }
//
//    th.join();
//}