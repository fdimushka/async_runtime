#ifndef AR_AWAITER_H
#define AR_AWAITER_H

#include <utility>

#include "ar/task.hpp"
#include "ar/coroutine.hpp"


namespace AsyncRuntime {
    /**
     * @class
     * @brief
     */
    template< class ResultType >
    class Awaiter
    {
    public:
        typedef typename ResultType::RetType Ret;
        typedef std::shared_ptr<ResultType> ResultPtr;
        typedef std::function<void(void*)> resume_cb_t;

        explicit Awaiter(const ResultPtr& result_, resume_cb_t resume_cb_, CoroutineHandler*  handler_ = nullptr) :
             result(result_),
             resume_coroutine_cb(std::move(resume_cb_)),
             coroutine_handler(handler_) { };


        Awaiter(const Awaiter& other) = delete;
        Awaiter& operator =(const Awaiter& other) = delete;
        Awaiter(Awaiter&& other) = delete;
        Awaiter& operator =(Awaiter&& other) = delete;
        ~Awaiter() = default;


        /**
         * @brief
         * @return
         */
        Ret Await();
    private:
        ResultPtr                                       result;
        CoroutineHandler*                               coroutine_handler;
        resume_cb_t                                     resume_coroutine_cb;
    };


    template< >
    inline
    typename Result<Ret>::RetType Awaiter<Result<Ret>>::Await() {
        if(coroutine_handler != nullptr) {
            if (result->Then(resume_coroutine_cb, coroutine_handler)) {
                //suspend this
                coroutine_handler->Suspend();
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
