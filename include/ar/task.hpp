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
#include "ar/resource.hpp"


namespace AsyncRuntime {
    class Task;
    class IExecutor;


    /**
     * @class
     * @brief
     */
    template<class Ret>
    class Result {
        friend class runtime;

    public:
        typedef std::function<void(Result<Ret> *, void *)> completed_cb_t;
        typedef Ret RetType;

        class CallbackControlBlock {
            friend Result;
        public:
            CallbackControlBlock(const completed_cb_t &cb, void *o = nullptr) : callback(cb), opaque(o) { }

            void operator () (Result<Ret> *result) {
                if (callback)
                    callback(result, opaque);
            }
        private:
            bool owner = true;
            completed_cb_t callback;
            void *opaque = nullptr;
        };

        explicit Result() : excepted(false) {
            resolved.store(false, std::memory_order_relaxed);
            future = promise.get_future();
        };


        template<typename T>
        explicit Result(T &&v) : excepted(false) {
            resolved.store(true, std::memory_order_relaxed);
            future = promise.get_future();
            promise.set_value(v);
        };


        Result(const Result &other) = delete;

        Result &operator=(const Result &other) = delete;

        Result(Result &&other) = delete;

        Result &operator=(Result &&other) = delete;

        ~Result() = default;


        /**
         * @brief
         * @param function
         */
        bool Then(const completed_cb_t &cb, void *opaque = nullptr) {
            std::lock_guard<std::mutex> lock(resolve_mutex);
            if (!resolved.load(std::memory_order_relaxed)) {
                callback_cb = std::make_shared<CallbackControlBlock>(cb, opaque);
                callback_cb->owner = false;
                return true;
            } else {
                return false;
            }
        }

        bool Then(const std::shared_ptr<CallbackControlBlock> & cb) {
            std::lock_guard<std::mutex> lock(resolve_mutex);
            if (!resolved.load(std::memory_order_relaxed)) {
                callback_cb = cb;
                return true;
            } else {
                return false;
            }
        }


        /**
         * @brief
         */
        Result<Ret> *Wait() {
            if (!resolved.load(std::memory_order_relaxed)) {
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
        void SetValue(T &&v) {
            std::lock_guard<std::mutex> lock(resolve_mutex);
            if (!excepted && !Resolved()) {
                resolved.store(true, std::memory_order_relaxed);
                promise.set_value(v);
                InvokeCallback();
            }
        }


        /**
         * @brief
         * @tparam T
         * @param v
         */
        template<typename T>
        void SetValue(T &v) {
            std::lock_guard<std::mutex> lock(resolve_mutex);
            if (!excepted && !Resolved()) {
                resolved.store(true, std::memory_order_relaxed);
                promise.set_value(v);
                InvokeCallback();
            }
        }


        /**
         * @brief
         * @tparam T
         * @param v
         */
        template<typename T>
        void SetValue(const T &v) {
            std::lock_guard<std::mutex> lock(resolve_mutex);
            if (!excepted && !Resolved()) {
                resolved.store(true, std::memory_order_relaxed);
                promise.set_value(v);
                InvokeCallback();
            }
        }


        /**
         * @brief
         * @param v
         */
        void SetValue() {
            std::lock_guard<std::mutex> lock(resolve_mutex);
            if (!excepted && !Resolved()) {
                resolved.store(true, std::memory_order_relaxed);
                promise.set_value();
                InvokeCallback();
            }
        }


        /**
         * @brief
         * @param __p
         */
        void SetException(std::exception_ptr e) {
            std::lock_guard<std::mutex> lock(resolve_mutex);
            resolved.store(true, std::memory_order_relaxed);
            promise.set_exception(e);
            excepted = true;
        }


        /**
         * @brief
         * @return
         */
        Ret Get() {
            if (future.valid()) {
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
        void InvokeCallback() {
            if (callback_cb) {
                if (callback_cb->owner) {
                    if ( callback_cb.use_count() > 1) {
                        callback_cb->operator()(this);
                    }
                } else {
                    callback_cb->operator()(this);
                }
            }
        }


        std::future<Ret> future;
        std::shared_ptr<CallbackControlBlock> callback_cb;
        //completed_cb_t completed_cb;
        //void *completed_opaque{};
        std::promise<Ret> promise;
        std::atomic_bool resolved{};
        std::mutex resolve_mutex;
        bool excepted;
    };


    /**
     * @brief
     */
    struct ExecutorState {
        IExecutor *executor = nullptr;
        ObjectID work_group = INVALID_OBJECT_ID;
        ObjectID processor = INVALID_OBJECT_ID;
    };


    /**
     * @class TaskImplBase
     * @brief Task interface
     */
    class Task {
        friend class runtime;
    public:
        struct LessThanByDelay {
            bool operator()(const Task *lhs, const Task *rhs) const {
                return lhs->GetDelay() > rhs->GetDelay();
            }
        };

        Task() : origin_id_(0), delayed(false), delay(0), start_time(TIMESTAMP_NOW_MICRO()) {};

        virtual ~Task() = default;

        virtual void Execute(const ExecutorState &executor) = 0;

        template<typename Rep, typename T>
        void SetDelay(T time) {
            if (time > 0) {
                delayed = true;
                delay = Timestamp::Cast<Rep, Timestamp::Micro>(time);
                start_time = TIMESTAMP_NOW_MICRO() + delay;
            }
        }

        void SetOriginId(uintptr_t origin_id);

        void SetExecutorState(const ExecutorState &executor) { executor_state = executor; }

        void SetWorkGroupExecutorState(const ObjectID &group);

        void SetProcessorExecutorState(const ObjectID &processor) { executor_state.processor = processor; }

        void SetExecutorExecutorState(IExecutor *executor) { executor_state.executor = executor; }


        [[nodiscard]] bool Delayed() const { return delayed; }

        [[nodiscard]] Timespan GetDelay() const;

        [[nodiscard]] uintptr_t GetOriginId() const { return origin_id_; }

        [[nodiscard]] const ExecutorState &GetExecutorState() const { return executor_state; }

#ifdef USE_TESTS
        Timespan delay;
#else
        protected:
            Timespan delay;
#endif

    protected:
        ExecutorState executor_state;
        uintptr_t origin_id_;
        bool delayed;
        Timespan start_time;
    };


    /**
     * @class Task
     * @brief Task container
     */
    template<class Callable>
    class TaskImpl : public Task {
        typedef typename std::result_of<Callable(const ExecutorState &)>::type return_type;
    public:
        explicit TaskImpl(Callable &&f) :
                Task(),
                fn(f),
                result(new Result<return_type>()) {};

        ~TaskImpl() override = default;


        void Execute(const ExecutorState &executor_) override {
            try {
                executor_state = executor_;
                Handle(result.get(), fn);
            } catch (...) {
                try {
                    result->SetException(std::current_exception());
                } catch (...) {}
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
        void Handle(Result<R> *r, F &&f) {
            auto res = f(executor_state);
            r->SetValue(res);
        }


        /**
         * @class handle void here
         * @tparam F
         * @param p
         * @param f
         */
        template<typename F>
        void Handle(Result<void> *r, F &&f) {
            f(executor_state);
            r->SetValue();
        }


        Callable fn;
        std::shared_ptr<Result<return_type>> result;
    };


    class DummyTaskImpl : public Task {
    public:
        explicit DummyTaskImpl() : result(new Result<void>()) {};

        ~DummyTaskImpl() override = default;


        void Execute(const ExecutorState &executor_) override {
            try {
                executor_state = executor_;
                result->SetValue();
            } catch (...) {
                try {
                    result->SetException(std::current_exception());
                } catch (...) {}
            }
        }


        std::shared_ptr<Result<void>> GetResult() {
            return result;
        }

    private:


        std::shared_ptr<Result<void>> result;
    };


    /**
     * @brief
     * @tparam Callable
     * @param f
     * @return
     */
    template<class Fn>
    inline TaskImpl<Fn> *MakeTask(Fn &&f) {
        return new TaskImpl(std::forward<Fn>(f));
    }


    /**
     * @brief
     * @return
     */
    inline DummyTaskImpl *MakeDummyTask() {
        return new DummyTaskImpl();
    }


    typedef std::shared_ptr<Result<void>> ResultVoidPtr;
}


#endif //AR_TASK_H
