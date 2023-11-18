#include "ar/ar.hpp"


using namespace AsyncRuntime;


void async_io(CoroutineHandler* handler, YieldVoid & yield) {
    //make input stream
    auto in_stream = std::make_shared<IOStream>();
    int res = 0;
    int fd = 0;

    yield();

    //async open file
    if( (res = Await(AsyncFsOpen("../../examples/io.cpp"), handler)) < 0 ) {
        std::cerr << "Error open file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
        return;
    }

    fd = res;

    //async read file
    if( (res = Await(AsyncFsRead(fd, in_stream), handler)) != IO_SUCCESS ) {
        std::cerr << "Error read file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
        return;
    }


    //async close file
    Await(AsyncFsClose(fd), handler);

    //make output stream with data from input stream
    auto out_stream = std::make_shared<IOStream>(in_stream->GetBuffer(), in_stream->GetBufferSize());

    //async open file
    if( (res = Await(AsyncFsOpen("tmp"), handler)) < 0 ) {
        std::cerr << "Error open file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
        return;
    }

    fd = res;

    //async write to file
    if( (res = Await(AsyncFsWrite(fd, out_stream), handler)) != IO_SUCCESS ) {
        std::cerr << "Error write to file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
        return;
    }

    //async close file
    Await(AsyncFsClose(fd), handler);
}


int main() {
    SetupRuntime();

    Coroutine coro = MakeCoroutine(&async_io);

    while (coro.Valid()) {
        Await(Async(coro));
    }

    Terminate();

    return 0;
}
