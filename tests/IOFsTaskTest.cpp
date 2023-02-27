#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/ar.hpp"

#include <string>


using namespace AsyncRuntime;


TEST_CASE( "io fs task open test", "[fs task]" ) {
    SetupRuntime();
    auto result = AsyncFsOpen("../../examples/io.cpp");
    result->Wait();
    int ret = result->Get();
    REQUIRE(ret > 0);
    AsyncFsClose(ret)->Wait();
}


TEST_CASE( "io fs task open error test", "[fs task]" ) {
    SetupRuntime();
    auto result = AsyncFsOpen("fail", O_RDWR , S_IRWXU);
    result->Wait();
    int ret = result->Get();
    REQUIRE(ret <= 0);
    AsyncFsClose(ret)->Wait();
}


TEST_CASE( "io fs task create file test", "[fs task]" ) {
    SetupRuntime();
    auto result = AsyncFsOpen("tmp");
    result->Wait();
    int ret = result->Get();
    REQUIRE(ret >= 0);
    AsyncFsClose(ret);

    auto in = std::fstream("tmp");
    REQUIRE(in.is_open());
    in.close();
}


TEST_CASE( "io fs task read test", "[fs task]" ) {
    SetupRuntime();
    auto stream = MakeStream();
    int ret = AsyncFsOpen("../../examples/io.cpp", O_RDWR , S_IRWXU)->Wait()->Get();
    REQUIRE(ret > 0);

    int fd = ret;

    ret = AsyncFsRead(fd, stream)->Wait()->Get();
    REQUIRE(ret == IO_SUCCESS);
    REQUIRE(stream->GetBufferSize() > 0);

    AsyncFsClose(fd)->Wait();
}


TEST_CASE( "io fs task write test", "[fs task]" ) {
    SetupRuntime();
    std::string str = "hello \n world!";
    auto stream = MakeStream(str.c_str(), str.size());
    int ret = AsyncFsOpen("tmp")->Wait()->Get();
    REQUIRE(ret >= 0);

    int fd = ret;

    ret = AsyncFsWrite(fd, stream)->Wait()->Get();
    REQUIRE(ret == IO_SUCCESS);

    auto in = std::ifstream("tmp");
    std::string str2;
    if(in) {
        std::ostringstream ss;
        ss << in.rdbuf(); // reading data
        str2 = ss.str();
    }
    REQUIRE(str2.compare(str) == 0);
    in.close();

    AsyncFsClose(fd)->Wait();
}