#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "numbers.h"


using namespace AsyncRuntime;
using namespace std::chrono_literals;



TEST_CASE("Numbers pack/unpack", "[numbers]" ) {
    std::vector<std::pair<uint16_t, uint16_t>> pairs_fixture = {
            {0, 0},
            {10, 10},
            {0, 100},
            {1, 256},
            {256, 256},
            {256, 0},
            {1, 1024},
            {1, 2000},
            {1, 3000},
    };

    for (const auto &pair :  pairs_fixture) {
        uint16_t x, y;
        uint32_t key = Numbers::Pack(pair.first, pair.second);
        Numbers::Unpack(key, x, y);
        REQUIRE(x == pair.first);
        REQUIRE(y == pair.second);
    }
}