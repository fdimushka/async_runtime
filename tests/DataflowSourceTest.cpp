#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#include "catch.hpp"
#include "ar/ar.hpp"
#include "ar/dataflow/source.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::Dataflow;

TEST_CASE( "add source check", "[source]" ) {
    Source source;
    SECTION( "add sources" ) {
        source.Add<int>("1", AsyncRuntime::Dataflow::kFIFO_BUFFER);
        source.Add<int>("2", AsyncRuntime::Dataflow::kTRIPLE_BUFFER);
        REQUIRE_THROWS_AS(source.Add<int>("1"), std::runtime_error);
    }

    SECTION( "at source" ) {
        source.Add<int>("1");
        auto source_port = source.At<int>("1");
        REQUIRE(source_port);
        REQUIRE_THROWS_AS(source.At<int>("2"), std::runtime_error);
    }
}

TEST_CASE( "r/w fifo source", "[source]" ) {
    Source source;

    SECTION( "sync r/w" ) {
        source.Add<int>("1");
        auto source_port = source.At<int>("1");
        for (int i = 0; i < 10; ++i) {
            source_port->Write(i);
        }

        for (int i = 0; i < 10; ++i) {
            REQUIRE(source_port->Read().value() == i);
        }
    }

    SECTION( "concurrent r/w" ) {
        source.Add<int>("1");
        auto source_port = source.At<int>("1");
        int read_count = 0;
        std::thread write_tread([&source_port](){
           for(int i = 1; i <= 100; ++i) {
               source_port->Write(i);
               std::this_thread::sleep_for(std::chrono::milliseconds(1));
           }
        });

        std::thread read_thread([&source_port, &read_count](){
            for(;;) {
               auto msg = source_port->Read();
               if (msg.has_value()) {
                   read_count++;
                   if (msg.value() == 100) {
                       break;
                   }
               }
               std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        write_tread.join();
        read_thread.join();
        REQUIRE(read_count == 100);
    }

    SECTION( "bounded source" ) {
        source.Add<int>("1", AsyncRuntime::Dataflow::kFIFO_BUFFER, 10);
        auto source_port = source.At<int>("1");

        for(int i = 0; i < 10; ++i) {
            int error = source_port->Write(i);
            REQUIRE(error == 0);
        }

        int v = 1;
        int error = source_port->Write(v);
        REQUIRE(error != 0);
    }
}

TEST_CASE( "r/w triple source", "[source]" ) {
    Source source;

    SECTION( "sync r/w" ) {
        source.Add<int>("1", AsyncRuntime::Dataflow::kTRIPLE_BUFFER);
        auto source_port = source.At<int>("1");
        for (int i = 0; i < 10; ++i) {
            int v = 100;
            source_port->Write(v);
        }

        auto msg = source_port->Read();
        if (msg.has_value()) {
            REQUIRE(msg.value() == 100);
        }
    }

    SECTION( "sync r/w 2" ) {
        source.Add<int>("1", AsyncRuntime::Dataflow::kTRIPLE_BUFFER);
        auto source_port = source.At<int>("1");
        for (int i = 0; i < 10; ++i) {
            source_port->Write(i);

            auto msg = source_port->Read();
            if (msg.has_value()) {
                REQUIRE(msg.value() == i);
            }
        }
    }
}