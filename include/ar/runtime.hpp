#ifndef AR_RUNTIME_H
#define AR_RUNTIME_H

#include "ar/work_steal_queue.h"
#include "ar/task.hpp"
#include "ar/notifier.hpp"
#include "ar/coroutine.hpp"
#include "ar/awaiter.hpp"
#include "ar/executor.hpp"


namespace AsyncRuntime {
    /**
     * @class Runtime
     * @brief runtime class
     */
    class Runtime {
        friend class Processor;
    public:
        static Runtime g_runtime;

        Runtime();
        ~Runtime();


        Runtime(const Runtime&) = delete;
        Runtime& operator =(const Runtime&) = delete;
        Runtime(Runtime&&) = delete;
        Runtime& operator =(Runtime&&) = delete;


        /**
         * @brief
         */
        void Setup(/*...*/);


        /**
         * @brief
         * @tparam Callable
         * @tparam Arguments
         * @param f
         * @param args
         * @return
         */
        template <  class Callable,
                    class... Arguments  >
        auto Async(Callable&& f, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>>;


        /**
         * @brief async call
         * @tparam Callable
         * @tparam Arguments
         * @param wg
         * @param f
         * @param args
         * @return
         */
        template <  class Callable,
                    class... Arguments  >
        auto Async(Executor* ex, Callable&& f, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>>;


        /**
         * @brief async call
         * @tparam CoroutineType
         */
        template<class CoroutineType>
        std::shared_ptr<Result<typename CoroutineType::RetType>> Async(CoroutineType & coroutine);


        /**
         * @brief
         * @tparam CoroutineType
         * @param coroutine
         * @return
         */
        template<class CoroutineType>
        std::shared_ptr<Result<typename CoroutineType::RetType>> Async(Executor* ex, CoroutineType & coroutine);


        /**
         * @brief async sleep
         * @tparam _Rep
         * @tparam _Period
         * @param rtime
         */
        template<typename Rep, typename Period>
        ResultVoidPtr AsyncSleep(const std::chrono::duration<Rep, Period>& rtime);


        /**
         * @brief
         * @tparam Ret
         * @param awaiter
         * @param context
         * @return
         */
        template<class Ret>
        Ret Await(std::shared_ptr<Result<Ret>> result);


        /**
         * @brief
         * @tparam Ret
         * @param awaiter
         * @param context
         * @return
         */
        template<class Ret, class YieldType>
        Ret Await(std::shared_ptr<Result<Ret>> result, YieldType& yield);


        /**
         * @brief
         * @tparam Ret
         * @param result
         * @param handler
         * @return
         */
        template<class Ret>
        Ret Await(std::shared_ptr<Result<Ret>> result, CoroutineHandler* handler);
    private:
        /**
         * @brief
         */
        Executor* CreateExecutor(const std::string& name,
                                 int max_processors_count=std::thread::hardware_concurrency());


        /**
         * @brief
         * @param task
         */
        void Post(Task * task);


        /**
         * @brief
         * @param task
         */
        void Post(Executor* ex, Task * task);


        /**
         * @brief
         */
        void CreateDefaultExecutors();


        std::unordered_map<ObjectID, Executor*>    executors;
        Executor*                                  main_executor;
        Executor*                                  io_executor;
    };


    template<class Callable, class... Arguments>
    auto Runtime::Async(Callable &&f, Arguments &&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>> {
        auto task = MakeTask(std::bind(std::forward<Callable>(f), std::forward<Arguments>(args)...));
        auto result = task->GetResult();
        Post(task);
        return result;
    }


    template<class Callable, class... Arguments>
    auto Runtime::Async(Executor* ex, Callable &&f, Arguments &&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>> {
        auto task = MakeTask(std::bind(std::forward<Callable>(f), std::forward<Arguments>(args)...));
        auto result = task->GetResult();
        Post(ex, task);
        return result;
    }


    template<class CoroutineType>
    std::shared_ptr<Result<typename CoroutineType::RetType>> Runtime::Async(CoroutineType & coroutine) {
        if(coroutine.GetState() != CoroutineState::kWaiting) {
            auto result = coroutine.GetResult();
            result->Wait();
        }

        auto task = coroutine.MakeExecTask();
        if(task != nullptr) {
            coroutine.MakeResult();
            auto result = coroutine.GetResult();
            Post(task);
            return result;
        }else{
            return {};
        }
    }


    template<class CoroutineType>
    std::shared_ptr<Result<typename CoroutineType::RetType>> Runtime::Async(Executor* ex, CoroutineType & coroutine) {
        if(coroutine.GetState() != CoroutineState::kWaiting) {
            auto result = coroutine.GetResult();
            result->Wait();
        }

        auto task = coroutine.MakeExecTask();
        if(task != nullptr) {
            coroutine.MakeResult();
            auto result = coroutine.GetResult();
            Post(ex, task);
            return result;
        }else{
            return {};
        }
    }


    template<typename Rep, typename Period>
    ResultVoidPtr Runtime::AsyncSleep(const std::chrono::duration<Rep, Period>& rtime) {
        return std::move(Async([](const std::chrono::duration<Rep, Period>& t){
            std::this_thread::sleep_for(t);
        }, rtime));
    }


    template<class Ret>
    Ret Runtime::Await(std::shared_ptr<Result<Ret>> result) {
        assert(result);

        result->Wait();
        return result->Get();
    }


    template<class Ret, class YieldType>
    Ret Runtime::Await(std::shared_ptr<Result<Ret>> result, YieldType& yield) {
        assert(result);

        auto handler = yield.coroutine_handler;
        if(handler != nullptr) {
            auto awaiter_resume_cb = [this](void* p) {
                if(p != nullptr) {
                    auto resumed_coroutine = (CoroutineHandler*)p;
                    auto task = resumed_coroutine->MakeExecTask();
                    if(task != nullptr) {
                        Post(task);
                    }
                }
            };

            Awaiter<Ret> awaiter(result, awaiter_resume_cb, handler);
            return awaiter.Await();
        }else{
            result->Wait();
        }

        return result->Get();
    }


    template<class Ret>
    Ret Runtime::Await(std::shared_ptr<Result<Ret>> result, CoroutineHandler* handler) {
        assert(result);
        assert(handler != nullptr);

        auto awaiter_resume_cb = [this](void* p) {
            if(p != nullptr) {
                auto resumed_coroutine = (CoroutineHandler*)p;
                auto task = resumed_coroutine->MakeExecTask();
                if(task != nullptr) {
                    Post(task);
                }
            }
        };

        Awaiter<Ret> awaiter(result, awaiter_resume_cb, handler);
        return awaiter.Await();
    }


    /**
     * @brief
     */
    inline void SetupRuntime(/*args...*/) {
        return Runtime::g_runtime.Setup();
    }


    /**
     * @brief
     * @tparam Callable
     * @tparam Arguments
     * @param f
     * @param args
     * @return
     */
    template <  class Callable,
                class... Arguments  >
    inline auto Async(Callable&& f, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>> {
        return Runtime::g_runtime.Async(std::forward<Callable>(f), std::forward<Arguments>(args)...);
    }


    /**
     * @brief async call
     * @tparam CoroutineType
     */
    template< class CoroutineType >
    inline std::shared_ptr<Result<typename CoroutineType::RetType>> Async(CoroutineType & coroutine) {
        return Runtime::g_runtime.Async(coroutine);
    }


    /**
     * @brief async sleep
     * @tparam _Rep
     * @tparam _Period
     * @param rtime
     */
    template< typename Rep, typename Period >
    inline ResultVoidPtr AsyncSleep(const std::chrono::duration<Rep, Period>& rtime) {
        return Runtime::g_runtime.AsyncSleep(rtime);
    }


    /**
     * @brief await
     * @tparam Ret
     * @param awaiter
     * @param context
     * @return
     */
    template< class Ret >
    inline Ret Await(std::shared_ptr<Result<Ret>> result) {
        return Runtime::g_runtime.Await(result);
    }


    /**
     * @brief await
     * @tparam Ret
     * @param awaiter
     * @param context
     * @return
     */
    template< class Ret, class YieldType >
    inline Ret Await(std::shared_ptr<Result<Ret>> result, YieldType& yield) {
        return Runtime::g_runtime.Await(result, yield);
    }


    /**
     * @brief await
     * @tparam Ret
     * @param awaiter
     * @param context
     * @return
     */
    template< class Ret >
    inline Ret Await(std::shared_ptr<Result<Ret>> result, CoroutineHandler* handler) {
        return Runtime::g_runtime.Await(result, handler);
    }
}


#endif //AR_RUNTIME_H
