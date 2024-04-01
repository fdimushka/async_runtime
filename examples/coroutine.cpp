#include <iostream>
#include "ar/ar.hpp"

static void async_fun(AsyncRuntime::CoroutineHandler* handler, AsyncRuntime::YieldVoid & yield) {
  yield();
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  char* buff = (char* )std::malloc(256);
  buff[0] = '0';
  std::free(buff);
}

int main() {
    AsyncRuntime::SetupRuntime();
    //std::shared_ptr<AsyncRuntime::Result<void>> res;
    int i = 0;
    for(;;) {
      std::cout << i << std::endl;
      AsyncRuntime::Coroutine<void> coroutine(&async_fun);
      auto res = AsyncRuntime::Async(coroutine);
      if (res) {
        res->Wait();
      }
      i++;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    AsyncRuntime::Terminate();
    return 0;
}

