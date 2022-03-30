#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/ar.hpp"


using namespace AsyncRuntime;


class CHANNEL_TEST_FRIEND {
public:

    void test1() {
        channel_test1.CallWatcher();
    }

    Channel<int>    channel_test1;
};


TEST_CASE( "Channel test", "[channel]" ) {
    CHANNEL_TEST_FRIEND test;
    test.test1();
}

