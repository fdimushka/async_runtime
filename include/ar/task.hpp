#ifndef AR_TASK_H
#define AR_TASK_H

#define BOOST_THREAD_PROVIDES_FUTURE 1
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION 1

#include "ar/object.hpp"
#include "ar/timestamp.hpp"

#include <boost/thread/future.hpp>
#include <boost/context/continuation.hpp>

#include <iostream>


namespace AsyncRuntime {
    class IExecutor;

    class task_pool;

    class task {
    public:
        struct execution_state {
            int64_t tag = INVALID_OBJECT_ID;
            int64_t work_group = INVALID_OBJECT_ID;
            int64_t processor = INVALID_OBJECT_ID;
            IExecutor *executor = nullptr;
        };

        struct less_than_by_delay_ptr {
            bool operator()(const task * lhs, const task * rhs) const { return lhs->get_delay() > rhs->get_delay(); }
        };

        task() noexcept: delay(0), created_at(TIMESTAMP_NOW_MICRO()) {};

        task(task const &) = delete;

        task &operator=(task const &) = delete;

        virtual ~task() = default;

        virtual void execute(const execution_state &state) = 0;

        template<typename Rep, typename T>
        void set_delay(T time) {
            delay = Timestamp::Cast<Rep, Timestamp::Micro>(time);
            created_at = TIMESTAMP_NOW_MICRO() + delay;
        }

        int64_t get_delay() const { return created_at - TIMESTAMP_NOW_MICRO(); }

        const execution_state &get_execution_state() const { return state; }

        void set_execution_state(const execution_state & new_state) { state = new_state; }

        void set_execution_state_wg(const int64_t & work_group) { state.work_group = work_group; }

        void set_execution_state_tag(const int64_t & tag) { state.tag = tag; }

        bool delayed() const { return delay > 0; }

        virtual bool resolved() { return false; }
    protected:
        execution_state state;
    private:
        int64_t delay;
        int64_t created_at;
    };

    template < typename T >
    using promise_t = boost::promise<T>;

    template < typename T >
    using future_t = boost::future<T>;

    template < typename T >
    using shared_future_t = boost::shared_future<T>;

//    template < typename T >
//    class task_future : public std::future< T > {
//    public:
//
//    private:
//    };
//
//
//    template < typename T >
//    class task_promise : public std::promise< T > {
//    public:
//        task_promise();
//
//    private:
//    };
//
//    template < typename T >
//    using promise_t = task_promise<T>;
//
//    template < typename T >
//    using future_t = task_future<T>;

    template< typename Fn >
    class base_task : public task {
        typedef typename std::result_of<Fn()>::type result_type;
        typedef promise_t<result_type> promise_type;
    public:
        explicit base_task(Fn &&f) noexcept: fn(f) { }

        ~base_task() override = default;

        void execute(const execution_state & state) override {
            try {
                task::state = state;
                invoke_fn(promise);
            } catch (std::exception & ex) {
                std::cerr << ex.what() << std::endl;
                promise.set_exception(ex);
            }
        }

        future_t<result_type> get_future() { return promise.get_future(); }
    private:
        template<typename T>
        inline void invoke_fn(promise_t<T> & p) {
            p.set_value(fn());
        }

        template<typename T = void>
        inline void invoke_fn(promise_t<void> & p) {
            fn();
            promise.set_value();
        }

        Fn fn;
        promise_type promise;
    };

    class dummy_task : public task {
        typedef promise_t<void> promise_type;
    public:
        dummy_task() = default;
        ~dummy_task() override = default;

        void execute(const execution_state & state) override {
            try {
                task::state = state;
                promise.set_value();
            } catch (std::exception & ex) {
                std::cerr << ex.what() << std::endl;
                promise.set_exception(ex);
            }
        }

        future_t<void> get_future() { return promise.get_future(); }
    private:
        promise_type promise;
    };

    template < class Fn >
    inline base_task<Fn>* make_task(Fn &&f) {
        return new base_task<Fn>(std::forward<Fn>(f));
    }

    inline dummy_task* make_dummy_task() {
        return new dummy_task();
    }

    template < typename T >
    future_t<T> make_resolved_future(T v) {
        promise_t<T> p;
        p.set_value(v);
        return p.get_future();
    }

    future_t<void> make_resolved_future();
}


#endif //AR_TASK_H
