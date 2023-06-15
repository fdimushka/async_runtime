#ifndef AR_AWAITER_H
#define AR_AWAITER_H

#include <utility>

#include "ar/task.hpp"
#include "ar/coroutine.hpp"


namespace AsyncRuntime::Awaiter {
    template<class Ret>
    Ret Await(std::shared_ptr<AsyncRuntime::Result<Ret>> result,
              std::function<void(AsyncRuntime::Result<Ret> *, void *)> resume_cb,
              CoroutineHandler* handler) {
        if (handler != nullptr) {
            if (result->Then(resume_cb, handler)) {
                //suspend this
                handler->Suspend();
                //resume this
            }else{
                result->Wait();
            }
        } else {
            result->Wait();
        }

        return result->Get();
    }
}


#endif //AR_AWAITER_H
