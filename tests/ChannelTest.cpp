#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/ar.hpp"


using namespace AsyncRuntime;


SCENARIO( "Channel test" ) {
    GIVEN("An empty channel") {
        Channel<int> channel(32);

        THEN("The size and capacity start at 0") {
            REQUIRE(channel.Size() == 0);
            REQUIRE(channel.Empty());
            REQUIRE(channel.Capacity() == 32);
        }


        WHEN("send") {
            channel.Send(0);
            THEN("The size changes") {
                REQUIRE(channel.Size() == 1);
            }
        }


        WHEN("receive") {
            srand (time(NULL));
            int i = std::rand();
            channel.Send(i);
            std::optional<int> res = channel.Receive();
            REQUIRE(res);
            THEN("The size changes") {
                REQUIRE(i == res);
            }
        }


        WHEN("full") {
            for(size_t i = 0; i < 33; ++i )
                channel.Send(0);

            THEN("The size changes") {
                REQUIRE(!channel.Send(0));
                REQUIRE(channel.Size() == channel.Capacity());
                REQUIRE(channel.Full());
                REQUIRE(!channel.Empty());
            }
        }


        WHEN("flush") {
            REQUIRE(channel.Empty());
            for(size_t i = 0; i < 10; ++i )
                channel.Send(0);

            REQUIRE(!channel.Empty());
            channel.Flush();

            THEN("The size changes") {
                REQUIRE(channel.Empty());
                REQUIRE(channel.Size() == 0);
            }
        }
    }
}


struct WatchCtx {
    bool called = false;
};


class CHANNEL_TEST_FRIEND {
public:
    void watcher_alloc_dealloc() {
        WatchCtx ctx;
        channel_watcher_alloc_dealloc.Watch(1, [](void* p) {
            auto *c = (WatchCtx*)p;
            c->called = true;
        }, &ctx);

        REQUIRE(channel_watcher_alloc_dealloc.watcher != nullptr);
        REQUIRE(channel_watcher_alloc_dealloc.watcher->id == 1);
        REQUIRE(channel_watcher_alloc_dealloc.watcher->data == std::addressof(ctx));
        channel_watcher_alloc_dealloc.Send(0);
        REQUIRE(channel_watcher_alloc_dealloc.watcher == nullptr);
        REQUIRE(ctx.called);
    }

    Channel<int>    channel_watcher_alloc_dealloc;
};



TEST_CASE( "Channel watch test", "[channel]" ) {
    srand (time(NULL));
    Channel<int> channel;
    WatchCtx ctx;
    channel.Watch(0, [](void* p) {
        auto *c = (WatchCtx*)p;
        c->called = true;
    }, &ctx);

    int v = std::rand()%100;
    channel.Send(v);

    REQUIRE(ctx.called);
    REQUIRE(channel.Receive().value() == v);
}


TEST_CASE( "Channel watch alloc/dealloc test", "[channel]" ) {
    CHANNEL_TEST_FRIEND test;
    test.watcher_alloc_dealloc();
}

