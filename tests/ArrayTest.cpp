#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include "catch.hpp"
#include "ar/array.hpp"

using namespace AsyncRuntime;

SCENARIO( "Array can be sized and resized" ) {
    GIVEN("An empty vector") {
        auto a = AtomicArray<int>(10);

        THEN("The size and capacity start at 0") {
            REQUIRE(a.size() == 0);
            REQUIRE(a.capacity() == 10);
        }


        WHEN( "store() is called" ) {
            a.store(0, 1);
            THEN( "The size changes" ) {
                REQUIRE(a.size() == 1);
                REQUIRE( a.capacity() >= 1 );
            }
        }


        WHEN( "load() is called" ) {
            a.store(0, 1);
            int ret = a.load(0 );
            THEN( "The array changes" ) {
                REQUIRE( ret == 1 );
            }
        }

        WHEN( "[] is called" ) {
            a.store(2, 1);
            int ret = a[2];
            THEN( "The array changes" ) {
                REQUIRE( ret == 1 );
            }
        }


        WHEN( "resize() is called" ) {
            a.store(1, 1);
            auto tmp = a.resize(a.capacity()-1, 0);
            tmp->store(11, 11);

            int ret1 = tmp->load(1);
            int ret11 = tmp->load(11);
            THEN( "The array changes" ) {
                REQUIRE( ret1 == 1 );
                REQUIRE( ret11 == 11 );
            }
        }
    }
}
