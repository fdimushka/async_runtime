#include "ar/ar.hpp"
#include "uv.h"


namespace AR = AsyncRuntime;


void async_func(AR::CoroutineHandler* handler, AR::YieldVoid & yield) {
    yield();
    AR::IOFsStreamPtr io_stream = std::make_shared<AR::IOFsStream>();
    AR::IOResult res = AR::Await(AR::AsyncFsOpen(io_stream, "../../examples/io.cpp"), handler);

    if(res == IO_SUCCESS) {
        AR::Await(AR::AsyncFsRead(io_stream));
        std::cout << io_stream->GetReadBufferSize() << std::endl;
    }else{
        std::cerr << "Error open file: " << res << std::endl;
    }
}


int main() {
    AR::SetupRuntime();

    AR::Coroutine coro = AR::MakeCoroutine(&async_func);

    while (coro.Valid()) {
        AR::Await(AR::Async(coro));
    }

    AR::Terminate();

    return 0;
}
