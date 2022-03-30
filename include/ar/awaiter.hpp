#ifndef AR_AWAITER_H
#define AR_AWAITER_H

#include <utility>

#include "ar/task.hpp"
#include "ar/coroutine.hpp"


namespace AsyncRuntime::Awaiter {
    typedef std::function<void(void*)> ResumeCb;


    template<class Ret, class Res>
    inline
    Ret Await(const std::shared_ptr<Res>& result, ResumeCb resume_cb, CoroutineHandler* handler = nullptr);


    template<class Ret>
    inline Ret Await(const std::shared_ptr<Result<Ret>>& result, ResumeCb resume_cb, CoroutineHandler* handler) {
        if(handler != nullptr) {
            if (result->Then(resume_cb, handler)) {
                //suspend this
                handler->Suspend();
                //resume this
            }else{
                result->Wait();
            }
        }else{
            result->Wait();
        }

        return result->Get();
    }
}


#endif //AR_AWAITER_H
