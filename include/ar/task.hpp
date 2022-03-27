#ifndef AR_TASK_H
#define AR_TASK_H

#include <iostream>
#include <future>
#include <functional>
#include <typeinfo>
#include <cstdio>
#include <utility>
#include <atomic>

#include "helper.hpp"
#include "object.hpp"


namespace AsyncRuntime {
    class Task;
    class Processor;
    class Executor;


    /**
     * @class
     * @brief
     */
    template<class Ret>
    class Result
    {
        friend class runtime;
    public:
        typedef std::function<void(void*)> resume_cb_t;

        explicit Result() : excepted(false) {
            resolved.store(false, std::memory_order_relaxed);
            future = promise.get_future();
        };


        Result(const Result& other) = delete;
        Result& operator =(const Result& other) = delete;
        Result(Result&& other) = delete;
        Result& operator =(Result&& other) = delete;


        ~Result() = default;


        /**
         * @brief
         * @param function
         */
        bool Then(const resume_cb_t& cb, void* opaque = nullptr) {
            std::lock_guard<std::mutex>   lock(resolve_mutex);
            if(!resolved.load(std::memory_order_relaxed)) {
                completed_opaque = opaque;
                completed_cb = cb;
                return true;
            }else{
                return false;
            }
        }


        /**
         * @brief
         */
        void Wait() {
            if(!resolved.load(std::memory_order_relaxed)) {
                if (future.valid())
                    future.wait();
            }
        }


        /**
         * @brief
         * @param v
         */
        template<typename T>
        void SetValue(T && v) {
            std::lock_guard<std::mutex>   lock(resolve_mutex);
            resolved.store(true, std::memory_order_relaxed);

            if(!excepted) {
                promise.set_value(v);

                if (completed_cb)
                    completed_cb(completed_opaque);

                completed_cb = nullptr;
            }
        }


        /**
         * @brief
         * @param v
         */
        void SetValue() {
            std::lock_guard<std::mutex>   lock(resolve_mutex);
            resolved.store(true, std::memory_order_relaxed);

            if(!excepted) {
                promise.set_value();

                if (completed_cb)
                    completed_cb(completed_opaque);

                completed_cb = nullptr;
            }
        }


        /**
         * @brief
         * @param __p
         */
        void SetException(std::exception_ptr e) {
            std::lock_guard<std::mutex>   lock(resolve_mutex);
            resolved.store(true, std::memory_order_relaxed);
            promise.set_exception(e);
            excepted = true;
        }


        /**
         * @brief
         * @return
         */
        Ret Get() {
            if(future.valid()) {
                return future.get();
            } else {
                throw std::runtime_error("invalid future");
            }
        }


        /**
         * @brief
         * @return
         */
        bool Valid() const { return future.valid(); }


        /**
         * @brief
         * @return
         */
        bool Resolved() const {
            bool res = resolved.load(std::memory_order_relaxed);
            return res;
        }
    protected:
        std::future<Ret>                    future;
        resume_cb_t                         completed_cb;
        void*                               completed_opaque;
        std::promise<Ret>                   promise;
        std::atomic_bool                    resolved;
        std::mutex                          resolve_mutex;
        bool                                excepted;
    };


    /**
     * @brief
     */
    struct ExecutorState {
        Executor*  executor = nullptr;
        Processor*  processor = nullptr;
    };


    /**
     * @class TaskImplBase
     * @brief Task interface
     */
    class Task
    {
        friend class runtime;
    public:
        Task() = default;
        virtual ~Task() = default;
        virtual void Execute(const ExecutorState& executor = ExecutorState()) = 0;


        void SetDesirableExecutor(const ExecutorState& executor_) { desirable_executor = executor_; }
        const ExecutorState& GetDesirableExecutor() const { return desirable_executor; }
    protected:
        ExecutorState executor;
        ExecutorState desirable_executor;
    };


    /**
     * @class Task
     * @brief Task container
     */
    template < class Callable  >
    class TaskImpl : public Task
    {
        typedef typename std::result_of<Callable(const ExecutorState&)>::type return_type;
    public:
        explicit TaskImpl(Callable&& f) :
                Task(),
                fn(f),
                result(new Result<return_type>() ){ };

        ~TaskImpl() override = default;


        void Execute(const ExecutorState& executor_) override {
            try {
                executor = executor_;
                Handle(result.get(), fn);
            } catch(...) {
                try {
                    result->SetException(std::current_exception());
                } catch(...) { }
            }
        }


        std::shared_ptr<Result<return_type>> GetResult() {
            return result;
        }
    private:
        /**
         * @brief handle non-void here
         * @tparam F
         * @tparam R
         * @param p
         * @param f
         */
        template<typename F, typename R>
        void Handle(Result<R>* r, F && f)
        {
            auto res = f(executor);
            r->SetValue(res);
        }


        /**
         * @class handle void here
         * @tparam F
         * @param p
         * @param f
         */
        template<typename F>
        void Handle(Result<void>* r, F && f)
        {
            f(executor);
            r->SetValue();
        }


        Callable                                                        fn;
        std::shared_ptr<Result<return_type>>                            result;
    };


    /**
     * @brief
     * @tparam Callable
     * @param f
     * @return
     */
    template<class Fn>
    TaskImpl<Fn>* MakeTask(Fn &&f) {
        return new TaskImpl(std::forward<Fn>(f));
    }


    typedef std::shared_ptr<Result<void>>   ResultVoidPtr;
}


#endif //AR_TASK_H
