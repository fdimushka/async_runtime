#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/stream.hpp"

#include <string>


using namespace AsyncRuntime;


class STREAM_TEST_FRIEND {
public:
    STREAM_TEST_FRIEND() {
        stream = MakeStream();
        REQUIRE(stream->seek == 0);
        REQUIRE(stream->length == 0);
        REQUIRE(stream->allocated_length == 0);
        REQUIRE(stream->buffer == nullptr);
        REQUIRE(stream->allocated == false);
    }


    STREAM_TEST_FRIEND(const char *buffer, size_t size) {
        stream = MakeStream(buffer, size);
        REQUIRE(stream->seek == 0);
        REQUIRE(stream->length == size);
        REQUIRE(stream->allocated_length == size);
        REQUIRE(std::string(stream->buffer, stream->length).compare(std::string(buffer, size)) == 0);
        REQUIRE(stream->allocated == true);
    }


    void read_test1() {
        REQUIRE(stream->allocated == false);
        std::vector<std::string> msgs = {"hello", "", "world"};
        size_t chunk_size = 1024;
        for(const auto &msg : msgs) {
            auto *buf = stream->Next();
            REQUIRE(buf != nullptr);
            REQUIRE(buf->len == chunk_size);
            memcpy(buf->base, msg.c_str(), msg.size());
            stream->length += msg.size();
        }

        REQUIRE(std::string(stream->buffer, stream->length).compare("helloworld") == 0);

        stream->Flush();
        REQUIRE(stream->seek == 0);
        REQUIRE(stream->length == 0);
        REQUIRE(stream->allocated_length == 0);
        REQUIRE(stream->buffer == nullptr);
    }


    void read_test2() {
        srand (time(NULL));
        REQUIRE(stream->allocated == false);

        size_t chunk_size = 1024;
        std::string msgs;
        msgs.resize(chunk_size*3);

        for(size_t i = 0; i < msgs.size(); ++i ) {
            msgs[i] = 97 + rand() % 26;
        }

        for(size_t i = 0; i < msgs.size(); i+=chunk_size) {
            std::string msg = msgs.substr (i,chunk_size);
            auto *buf = stream->Next(chunk_size);
            REQUIRE(buf != nullptr);
            REQUIRE(buf->len == chunk_size);
            memcpy(buf->base, msg.c_str(), msg.size());
            stream->length += msg.size();
        }

        REQUIRE(std::string(stream->buffer, stream->length).compare(msgs) == 0);

        stream->Flush();
        REQUIRE(stream->seek == 0);
        REQUIRE(stream->length == 0);
        REQUIRE(stream->allocated_length == 0);
        REQUIRE(stream->buffer == nullptr);
    }


    void read_test3() {
        srand (time(NULL));
        REQUIRE(stream->allocated == false);

        std::string msgs;
        msgs.resize(1024*3);

        for(size_t i = 0; i < msgs.size(); ++i ) {
            msgs[i] = 97 + rand() % 26;
        }

        size_t i = 0;
        while (i < msgs.size()) {
            size_t size = std::min(1124 + rand() % 100, (int)(msgs.size() - i));
            std::string msg = msgs.substr (i,size);
            auto *buf = stream->Next(size);
            REQUIRE(buf != nullptr);
            REQUIRE(buf->len == size);
            memcpy(buf->base, msg.c_str(), msg.size());
            stream->length += msg.size();
            i += size;
        }

        REQUIRE(std::string(stream->buffer, stream->length).compare(msgs) == 0);

        stream->Flush();
        REQUIRE(stream->seek == 0);
        REQUIRE(stream->length == 0);
        REQUIRE(stream->allocated_length == 0);
        REQUIRE(stream->buffer == nullptr);
    }


    void begin_test() {
        stream->seek = 100;
        stream->Begin();
        REQUIRE(stream->seek == 0);
    }


    IOStreamPtr stream;
};


TEST_CASE( "Stream test read", "[stream]" ) {
    STREAM_TEST_FRIEND test;
    test.read_test1();
    test.read_test2();
    test.read_test3();
}


TEST_CASE( "Stream test write", "[stream]" ) {
    std::string str = "hello world";
    STREAM_TEST_FRIEND test(str.c_str(), str.size());
}


TEST_CASE( "Stream test begin", "[stream]" ) {
    STREAM_TEST_FRIEND test;
    test.begin_test();
}