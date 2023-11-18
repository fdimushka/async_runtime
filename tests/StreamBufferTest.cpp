#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/stream_buffer.hpp"

#include <string>
#include <ostream>


using namespace AsyncRuntime;


TEST_CASE( "Stream buffer test", "[stream]" ) {
    StreamBuffer sb;
    std::ostream out(&sb);

    SECTION( "write test" ) {
        out << "hello world";
        REQUIRE(sb.size() == 11);
        REQUIRE(std::string(sb.data(), sb.size()) == "hello world");
    }

    SECTION( "write 2 test" ) {
        int size = 1024*1024*10;
        char* msg = (char*)std::malloc(size);
        out.write(msg, size);
        REQUIRE(sb.size() == size);
        free(msg);
    }

    SECTION( "consume test" ) {
        out << "hello world";
        sb.consume(6);
        REQUIRE(sb.size() == 5);
        REQUIRE(std::string(sb.data(), sb.size()) == "world");
        out << "hello";
        REQUIRE(sb.size() == 10);
        REQUIRE(std::string(sb.data(), sb.size()) == "worldhello");
    }

    SECTION( "prepare test" ) {
        sb.prepare(1024);
        REQUIRE(sb.capacity() == 1024);
    }
}
