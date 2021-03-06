#ifndef AR_TASK_H
#define AR_TASK_H

#include <iostream>
#include <future>
#include <functional>
#include <typeinfo>
#include <cstdio>
#include <utility>
#include <atomic>

#include "ar/helper.hpp"
#include "ar/object.hpp"
#include "ar/logger.hpp"
#include "ar/timestamp.hpp"


namespace AsyncRuntime {
    class Task;
    class Processor;
    class IExecutor;


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
        typedef Ret RetType;

        explicit Result() : excepted(false) {
            resolved.store(false, std::memory_order_relaxed);
            future = promise.get_future();
        };


        template<typename T>
        explicit Result(T && v) : excepted(false) {
            resolved.store(true, std::memory_order_relaxed);
            future = promise.get_future();
            promise.set_value(v);
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
        Result<Ret>* Wait() {
            if(!resolved.load(std::memory_order_relaxed)) {
                if (future.valid())
                    future.wait();
            }

            return this;
        }


        /**
         * @brief
         * @param v
         */
        template<typename T>
        void SetValue(T && v) {
            std::lock_guard<std::mutex>   lock(resolve_mutex);
            if(!excepted && !Resolved()) {
                resolved.store(true, std::memory_order_relaxed);
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
            if(!excepted && !Resolved()) {
                resolved.store(true, std::memory_order_relaxed);
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
        [[nodiscard]] bool Valid() const { return future.valid(); }


        /**
         * @brief
         * @return
         */
        [[nodiscard]] bool Resolved() const {
            bool res = resolved.load(std::memory_order_relaxed);
            return res;
        }
    protected:
        std::future<Ret>                    future;
        resume_cb_t                         completed_cb;
        void*                               completed_opaque{};
        std::promise<Ret>                   promise;
        std::atomic_bool                    resolved{};
        std::mutex                          resolve_mutex;
        bool                                excepted;
    };


    /**
     * @brief
     */
    struct ExecutorState {
        IExecutor*  executor = nullptr;
        Processor*  processor = nullptr;
        void*       data = nullptr;
    };


    /**
     * @class TaskImplBase
     * @brief Task interface
     */
    class Task
    {
        friend class runtime;
    public:
        struct LessThanByDelay
        {
            bool operator()(const Task* lhs, const Task* rhs) const {
                return lhs->GetDelay() > rhs->GetDelay();
            }
        };

        Task() : origin_id_(0), delayed(false), delay(0), start_time(TIMESTAMP_NOW_MICRO()) {};
        virtual ~Task() = default;
        virtual void Execute(const ExecutorState& executor) = 0;


        template<typename Rep, typename T>
        void SetDelay(T time) {
            if(time > 0) {
                delayed = true;
                delay = Timestamp::Cast<Rep, Timestamp::Micro>(time);
                start_time = TIMESTAMP_NOW_MICRO() + delay;
            }
        }


        [[nodiscard]] bool Delayed() const { return delayed; }
        [[nodiscard]] Timespan GetDelay() const {
            if(!delayed) return 0;
            return start_time - TIMESTAMP_NOW_MICRO();
        }


        void SetOriginId(uintptr_t origin_id) { origin_id_ = origin_id; }
        [[nodiscard]] uintptr_t GetOriginId() const { return origin_id_; }


        void SetDesirableExecutor(const ExecutorState& executor_) { desirable_executor = executor_; }
        [[nodiscard]] const ExecutorState& GetDesirableExecutor() const { return desirable_executor; }

#ifdef USE_TESTS
        Timespan delay;
#else
    protected:
        Timespan delay;
#endif

    protected:
        ExecutorState executor;
        ExecutorState desirable_executor;
        uintptr_t origin_id_;
        bool delayed;
        Timespan start_time;
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


    class DummyTaskImpl : public Task
    {
    public:
        explicit DummyTaskImpl() : result(new Result<void>() ){ };
        ~DummyTaskImpl() override = default;


        void Execute(const ExecutorState& executor_) override {
            try {
                executor = executor_;
                result->SetValue();
            } catch(...) {
                try {
                    result->SetException(std::current_exception());
                } catch(...) { }
            }
        }


        std::shared_ptr<Result<void>> GetResult() {
            return result;
        }
    private:


        std::shared_ptr<Result<void>>                                   result;
    };


    /**
     * @brief
     * @tparam Callable
     * @param f
     * @return
     */
    template<class Fn>
    inline TaskImpl<Fn>* MakeTask(Fn &&f) {
        return new TaskImpl(std::forward<Fn>(f));
    }


    /**
     * @brief
     * @return
     */
    inline DummyTaskImpl* MakeDummyTask() {
        return new DummyTaskImpl();
    }


    typedef std::shared_ptr<Result<void>>   ResultVoidPtr;
}


#endif //AR_TASK_H
