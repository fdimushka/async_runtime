#include "ar/ar.hpp"


using namespace AsyncRuntime;


void async_io(CoroutineHandler* handler, YieldVoid & yield) {
    //make input stream
    auto in_stream = MakeStream();
    int res = 0;

    yield();

    //async open file
    if( (res = Await(AsyncFsOpen(in_stream, "../../examples/io.cpp"), handler)) != IO_SUCCESS ) {
        std::cerr << "Error open file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
        return;
    }

    //async read file
    if( (res = Await(AsyncFsRead(in_stream), handler)) != IO_SUCCESS ) {
        std::cerr << "Error read file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
        return;
    }

    //async close file
    Await(AsyncFsClose(in_stream), handler);

    //make output stream with data from input stream
    auto out_stream = MakeStream(in_stream->GetBuffer(), in_stream->GetBufferSize());

    //async open file
    if( (res = Await(AsyncFsOpen(out_stream, "tmp"), handler)) != IO_SUCCESS ) {
        std::cerr << "Error open file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
        return;
    }

    //async write to file
    if( (res = Await(AsyncFsWrite(out_stream), handler)) != IO_SUCCESS ) {
        std::cerr << "Error write to file: " << FSErrorName(res) << ' ' << FSErrorMsg(res) << std::endl;
        return;
    }

    //async close file
    Await(AsyncFsClose(out_stream), handler);
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
