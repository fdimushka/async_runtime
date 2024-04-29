#include <iostream>
//#include "ar/ar.hpp"
//#include "ar/dataflow/dataflow.hpp"
//#include "ar/tbb_executor.hpp"
//#include "ar/timestamp.hpp"
#include <queue>

#define BOOST_THREAD_PROVIDES_FUTURE 1
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION 1

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/context/continuation.hpp>
#include <boost/function_types/result_type.hpp>
#include "ar/timestamp.hpp"

#include <oneapi/tbb.h>

using namespace AsyncRuntime;
using namespace std::chrono_literals;
namespace ctx = boost::context;

class task {
public:
    struct execution_state {
        int64_t tag = 0;
        int64_t work_group = 0;
        int64_t processor = 0;
        void *executor = nullptr;
    };

    struct less_than_by_Delay {
        bool operator()(const task *lhs, const task *rhs) const { return lhs->delay > rhs->delay; }
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

    int64_t get_delay() const { return delay; }

    const execution_state &get_execution_state() const { return state; }

    bool delayed() const { return delay > 0; }

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

template < class Fn >
inline std::shared_ptr<base_task<Fn>> make_task_shared_ptr(Fn &&f) {
    return std::make_shared<base_task<Fn>>(std::forward<Fn>(f));
}

template< typename T >
class coroutine;

template< typename T >
class coroutine_task;

class coroutine_handler : public std::enable_shared_from_this<coroutine_handler> {
public:
    virtual ~coroutine_handler() = default;

    virtual bool is_completed() = 0;

    virtual void suspend() = 0;

    virtual void suspend_with(std::function<void(coroutine_handler *handler)> fn) = 0;

    virtual std::shared_ptr<task> resume_task() = 0;
};

template< typename T >
class yield {
    friend class coroutine<T>;
    friend class coroutine_task<T>;
public:
    yield() noexcept = default;

    explicit yield(ctx::continuation && c) : continuation(std::move(c)) {}

    yield( yield && other) noexcept = default;
    yield( yield const& other) noexcept = delete;
    yield & operator=( yield const& other) noexcept = delete;

    void operator ()(const T & v) {
        continuation = continuation.resume_with([this, &v](ctx::continuation && c) {
            promise.set_value(v);
            return std::move(c);
        });
    }

    future_t<T> get_future() { return promise.get_future(); }
private:
    promise_t<T> promise;
    ctx::continuation continuation;
};

template< >
class yield<void> {
    friend class coroutine<void>;
    friend class coroutine_task<void>;
public:
    yield() noexcept = default;

    explicit yield(ctx::continuation && c) : continuation(std::move(c)) {}

    yield( yield && other) noexcept = default;
    yield( yield const& other) noexcept = delete;
    yield & operator=( yield const& other) noexcept = delete;

    void operator ()() {
        continuation = continuation.resume_with([this](ctx::continuation && c) {
            promise.set_value();
            return std::move(c);
        });
    }

    future_t<void> get_future() { return promise.get_future(); }
private:
    promise_t<void> promise;
    ctx::continuation continuation;
};

template< typename Ret >
class coroutine : public coroutine_handler {
    typedef std::function<Ret(coroutine_handler *, yield<Ret> &)> Fn;
    typedef coroutine<Ret> coroutine_t;
    typedef yield<Ret> yield_t;
    friend class coroutine_task<Ret>;
public:
    coroutine() noexcept = default;

    explicit coroutine(Fn && f );

    coroutine( coroutine && other) noexcept = default;
    coroutine( coroutine const& other) noexcept = delete;
    coroutine & operator=( coroutine const& other) noexcept = delete;

    ~coroutine() override = default;

    bool is_completed() final { return !continuation; }

    void suspend() final {
        y.continuation = y.continuation.resume();
    }

    void suspend_with(std::function<void(coroutine_handler *handler)> callback) final {
        y.continuation = y.continuation.resume_with([=](ctx::continuation && c) {
            callback(this);
            return std::move(c);
        });
    }

    void resume() {
        std::unique_lock<std::mutex> lock(m);
        continuation = continuation.resume();
    }

    std::shared_ptr<task> resume_task() final {
        return std::static_pointer_cast<task>(std::make_shared<coroutine_task<Ret>>(get_ptr()));
    }

    future_t<Ret> get_future() { return y.get_future(); }

    std::shared_ptr<coroutine_t> get_ptr() { return std::static_pointer_cast<coroutine_t>(shared_from_this()); };

    void init_promise() { y.promise = {}; }
private:
    static Ret invoke(std::shared_ptr<coroutine_t> coroutine, yield_t & yield, coroutine::Fn && f);

    Fn fn;
    task::execution_state state;
    std::mutex m;
    ctx::continuation continuation;
    yield_t y;
};

template<typename Ret>
coroutine<Ret>::coroutine(coroutine::Fn &&f) : fn(f) {
    continuation = ctx::continuation(ctx::callcc([this](ctx::continuation && c) {
        y.continuation = std::move(c);
        y();
        Ret v = invoke(get_ptr(), y, std::forward<Fn>(fn));
        y.promise.set_value(v);
        return std::move(y.continuation);
    }));
}

template<>
coroutine<void>::coroutine(coroutine::Fn &&f) : fn(f) {
    continuation = ctx::continuation(ctx::callcc([this](ctx::continuation && c) {
        y.continuation = std::move(c);
        y();
        invoke(get_ptr(), y, std::forward<Fn>(fn));
        y.promise.set_value();
        return std::move(y.continuation);
    }));
}

template<typename Ret>
Ret coroutine<Ret>::invoke(std::shared_ptr<coroutine_t> coroutine, yield_t & yield, coroutine::Fn && f) {
    return f(coroutine.get(), yield);
}


template< typename Ret >
class coroutine_task : public task {
public:
    explicit coroutine_task(std::shared_ptr<coroutine<Ret>> coroutine) noexcept: coro(coroutine) { }

    ~coroutine_task() override = default;

    void execute(const execution_state & state) override {
        try {
            task::state = state;
            coro->resume();
        } catch (std::exception & ex) {
            std::cerr << ex.what() << std::endl;
        }
    }

    future_t<Ret> get_future() { return coro->get_future(); }
private:
    std::shared_ptr<coroutine<Ret>> coro;
};

template <typename Ret = void, typename Fn, typename ...Arguments>
std::shared_ptr<coroutine<Ret>> make_coroutine(Fn &&fn, Arguments &&... args) {
    return std::make_shared<coroutine<Ret>>(std::bind(std::forward<Fn>(fn), std::placeholders::_1, std::placeholders::_2, std::forward<Arguments>(args)...));
}

template <typename Ret = void, typename Fn>
std::shared_ptr<coroutine<Ret>> make_coroutine(Fn &&fn) {
    return std::make_shared<coroutine<Ret>>(std::forward<Fn>(fn));
}

static tbb::task_arena task_arena;

template< class Callable, class... Arguments >
auto Async(Callable && f, Arguments &&... args) -> future_t<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))> {
    auto task = make_task_shared_ptr(std::bind(std::forward<Callable>(f), std::forward<Arguments>(args)...));
    task_arena.enqueue([task]() {
        task::execution_state state;
        task->execute(state);
    });
    return task->get_future();
}

void enqueue_task(const std::shared_ptr<task> & task) {
    task_arena.enqueue([task]() {
        task::execution_state state;
        task->execute(state);
    });
}

template< typename Ret >
future_t<Ret> Async(const std::shared_ptr<coroutine<Ret>> & coroutine) {
    coroutine->init_promise();
    auto task = std::make_shared<coroutine_task<Ret>>(coroutine);
    enqueue_task(task);
    return task->get_future();
}

template< class Ret >
Ret Await(future_t<Ret> && future) {
    future.wait();
    return future.get();
}

template< class Ret >
Ret Await(future_t<Ret> && future, coroutine_handler *handler) {
    if (!future.has_value() && !future.has_exception()) {
        future_t<Ret> done_future;
        handler->suspend_with([&future, &done_future](coroutine_handler *handler) {
            done_future = future.then(boost::launch::sync, [handler](future_t<Ret> f) {
                auto task = handler->resume_task();
                enqueue_task(task);
                return f.get();
            });
        });
        return done_future.get();
    } else {
        return future.get();
    }
}

int foo(coroutine_handler *handler, yield<int> & yield) {
    int i = 0;
    for (;;) {
        i++;
        yield(i);
        if (i > 1000) {
            break;
        }
    }
    return 300;
}

void foo2(coroutine_handler *handler, yield<void> & yield, int index) {
    int i = 0;
    for (;;) {
        i = Await(Async([&i](){
            i++;
            return i;
        }), handler);

        //yield();
        std::cout << "foo" << index << " " << i << std::endl;
        if (i > 200) {
            std::cout << "end" << std::endl;
            break;
        }
    }
}

int main() {
//
//    AsyncRuntime::Logger::s_logger.SetStd();
//    AsyncRuntime::SetupRuntime();

//    boost::promise<int> prm;
//
//    {
//        boost::future<int> f1 = prm.get_future();
//
//        auto f = f1.then(boost::launch::sync, [](boost::future<int> f) {
//            if (f.valid()) {
//                std::cout << f.get() << std::endl;
//            }
//        });
//    }
//
//
//    std::thread th([&](){
//        sleep(1);
//        prm.set_value(100);
//    });
//
//    th.join();

    //boost::future<int> f = task.get_future();

    //std::cout << f.get() << std::endl;
    task_arena.initialize(tbb::task_arena::constraints(0, 8));

    int i = 0;
    auto coro = make_coroutine(&foo2, i);
    Await(Async(coro));

    task_arena.terminate();

    return 0;
}

