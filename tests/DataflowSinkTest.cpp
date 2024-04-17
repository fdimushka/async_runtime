#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#include "catch.hpp"
#include "ar/ar.hpp"
#include "ar/dataflow/sink.hpp"
#include "ar/dataflow/source.hpp"

using namespace AsyncRuntime;
using namespace AsyncRuntime::Dataflow;

TEST_CASE( "add sink", "[sink]" ) {
    Sink sink;
    SECTION( "add sink" ) {
        sink.Add<int>("1");
        sink.Add<int>("2");
        REQUIRE_THROWS_AS(sink.Add<int>("1"), std::runtime_error);
    }

    SECTION( "at sink" ) {
        sink.Add<int>("1");
        auto sink_port = sink.At<int>("1");
        REQUIRE(sink_port);
        REQUIRE_THROWS_AS(sink.At<int>("2"), std::runtime_error);
    }
}


TEST_CASE( "sink consumers", "[sink]" ) {
    Sink sink;
    SECTION( "connect/disconnect consumers positive" ) {
        sink.Add<int>("output");
        Source src1, src2;

        src1.Add<int>("input");
        src2.Add<int>("input");

        REQUIRE(sink.At<int>("output")->ConsumersCount() == 0);
        sink.At<int>("output")->Connect(src1.At<int>("input"));
        sink.At<int>("output")->Connect(src2.At<int>("input"));
        REQUIRE(sink.At<int>("output")->ConsumersCount() == 2);
        sink.At<int>("output")->Disconnect(src1.At<int>("input"));
        sink.At<int>("output")->Disconnect(src2.At<int>("input"));
        REQUIRE(sink.At<int>("output")->ConsumersCount() == 0);
    }

    SECTION( "connect/disconnect consumers negative" ) {
        sink.Add<int>("output");
        Source src;
        src.Add<int>("input");
        sink.At<int>("output")->Connect(src.At<int>("input"));
        REQUIRE_THROWS_AS(sink.At<int>("output")->Connect(src.At<int>("input")), std::runtime_error);
    }

    SECTION( "subscribe/unsubscribe users" ) {
        PortUser user;
        sink.Add<int>("output");
        REQUIRE(sink.SubscribersEmpty() == true);
        sink.Subscribe("output", &user);
        sink.Subscribe("output", &user);
        sink.Subscribe("output", &user);
        REQUIRE(sink.SubscribersEmpty() == false);
        sink.Unsubscribe("output", &user);
        REQUIRE(sink.SubscribersEmpty() == true);
    }
}

TEST_CASE( "write to sink", "[sink]" ) {
    Sink sink;
    SECTION( "write positive" ) {
        sink.Add<int>("output");
        sink.At<int>("output")->Send(100);
    }

    SECTION( "write/read sink->source positive" ) {
        Source src;
        src.Add<int>("input");
        sink.Add<int>("output");
        auto sink_port = sink.At<int>("output");
        auto src_port = src.At<int>("input");
        sink_port->Connect(src_port);
        for (int i = 0; i < 100; ++i) {
            sink_port->Send(i);
        }

        for (int i = 0; i < 100; ++i) {
            auto msg = src_port->Read();
            if (msg.has_value()) {
                REQUIRE(msg.value() == i);
            }
        }
    }

    SECTION( "remove consumer after send" ) {
        sink.Add<int>("output");
        auto sink_port = sink.At<int>("output");

        {
            Source src;
            src.Add<int>("input");
            auto src_port = src.At<int>("input");
            sink_port->Connect(src_port);
            sink_port->Send(100);
            REQUIRE(sink_port->ConsumersCount() == 1);
        }

        sink_port->Send(100);
        REQUIRE(sink_port->ConsumersCount() == 0);
    }
}
