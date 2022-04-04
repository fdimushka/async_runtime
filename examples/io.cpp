#include "ar/ar.hpp"


namespace AR = AsyncRuntime;


void async_func(AR::CoroutineHandler* handler, AR::YieldVoid & yield) {
    yield();
    AR::IOFsStreamPtr in_stream = std::make_shared<AR::IOFsStream>();
    AR::IOResult res = AR::Await(AR::AsyncFsOpen(in_stream, "../../examples/runtime.cpp"), handler);

    if(res == IO_SUCCESS) {
        if(IO_SUCCESS == AR::Await(AR::AsyncFsRead(in_stream), handler)) {
            AR::IOFsStreamPtr out_stream = std::make_shared<AR::IOFsStream>(in_stream->GetReadBuffer(),
                                                                            in_stream->GetReadBufferSize());

            AR::Await(AR::AsyncFsClose(in_stream), handler);

            if (IO_SUCCESS == AR::Await(AR::AsyncFsOpen(out_stream, "tmp"), handler)) {
                AR::Await(AR::AsyncFsWrite(out_stream), handler);
                AR::Await(AR::AsyncFsClose(out_stream), handler);
            }
        }else{
            AR::Await(AR::AsyncFsClose(in_stream), handler);
        }
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
