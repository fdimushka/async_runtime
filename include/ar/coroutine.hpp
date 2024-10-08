#ifndef AR_COROUTINE_H
#define AR_COROUTINE_H

#include <iterator>
#include <type_traits>

#include "ar/task.hpp"

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/context/continuation.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/pool/simple_segregated_storage.hpp>

namespace AsyncRuntime {
    namespace ctx = boost::context;

    //struct stack_pool {};
    //static boost::singleton_pool<stack_pool, sizeof(int)> _stacks_pool;

    template< typename traitsT >
    class basic_fixedsize_stack {
    private:
        std::size_t     size_;

    public:
        typedef traitsT traits_type;

        basic_fixedsize_stack( std::size_t size = traits_type::default_size() ) BOOST_NOEXCEPT_OR_NOTHROW :
                size_( size) {
        }

        ctx::stack_context allocate() {
#if defined(BOOST_CONTEXT_USE_MAP_STACK)
            void * vp = ::mmap( 0, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_STACK, -1, 0);
        if ( vp == MAP_FAILED) {
            throw std::bad_alloc();
        }
#else
            void * vp = std::malloc( size_);
            if ( ! vp) {
                throw std::bad_alloc();
            }
#endif
            ctx::stack_context sctx;
            sctx.size = size_;
            sctx.sp = static_cast< char * >( vp) + sctx.size;
#if defined(BOOST_USE_VALGRIND)
            sctx.valgrind_stack_id = VALGRIND_STACK_REGISTER( sctx.sp, vp);
#endif
            return sctx;
        }

        void deallocate( ctx::stack_context & sctx) BOOST_NOEXCEPT_OR_NOTHROW {
            BOOST_ASSERT( sctx.sp);

#if defined(BOOST_USE_VALGRIND)
            VALGRIND_STACK_DEREGISTER( sctx.valgrind_stack_id);
#endif
            void * vp = static_cast< char * >( sctx.sp) - sctx.size;
#if defined(BOOST_CONTEXT_USE_MAP_STACK)
            ::munmap( vp, sctx.size);
#else
            std::free( vp);
#endif
        }
    };

    template< typename T >
    class coroutine;

    template< typename T >
    class coroutine_task;

    class coroutine_handler : public std::enable_shared_from_this<coroutine_handler> {
    public:
        coroutine_handler() {
            create();
        }

        virtual ~coroutine_handler() {
            destroy();
        }

        virtual bool is_completed() = 0;

        virtual void suspend() = 0;

        virtual void suspend_with(std::function<void(coroutine_handler *handler)> fn) = 0;

        virtual task *resume_task() = 0;

        const task::execution_state &get_execution_state() const { return execution_state; }

        void set_execution_state(const task::execution_state & new_state) { execution_state = new_state; }

        void set_execution_state_wg(const int64_t & work_group) {
            execution_state.work_group = work_group;
            execution_state.processor = INVALID_OBJECT_ID;
        }

        void set_execution_state_tag(const int64_t & tag) {
            execution_state.tag = tag;
            execution_state.processor = INVALID_OBJECT_ID;
        }
    protected:
        void create();

        void destroy();

        task::execution_state execution_state;
    };

    template< typename T >
    class yield {
        friend class coroutine<T>;
        friend class coroutine_task<T>;
    public:
        yield() = default;

        //explicit yield(ctx::continuation && c) : continuation(std::move(c)) {}

        yield( yield && other) = default;
        yield( yield const& other) = delete;
        yield & operator=( yield const& other) = delete;

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
        yield() = default;

        //explicit yield(ctx::continuation && c) : continuation(std::move(c)) {}

        yield( yield && other) = default;
        yield( yield const& other) = delete;
        yield & operator=( yield const& other) = delete;

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
        typedef basic_fixedsize_stack< ctx::stack_traits >  fixedsize_stack;
    public:
        coroutine() = default;

        explicit coroutine(coroutine::Fn &&f) : fn(f), end{false} {
            continuation = ctx::continuation(ctx::callcc(std::allocator_arg, fixedsize_stack(), [this](ctx::continuation &&c) {
                y.continuation = std::move(c);
                y.continuation = y.continuation.resume();
                call();
                end.store(true, std::memory_order_relaxed);
                return std::move(y.continuation);
            }));
        }

        coroutine( coroutine && other) = default;
        coroutine( coroutine const& other) = delete;
        coroutine & operator=( coroutine const& other) = delete;

        ~coroutine() override = default;

        bool is_completed() final { return end.load(std::memory_order_relaxed) || !continuation; }

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
            if (!is_completed()) {
                continuation = continuation.resume();
            } else {
                y.promise.set_exception(std::runtime_error("coroutine is completed"));
            }
        }

        task *resume_task() final {
            task *coro_task = new coroutine_task<Ret>(get_ptr());
            coro_task->set_execution_state(execution_state);
            return coro_task;
        }

        future_t<Ret> get_future() { return y.get_future(); }

        std::shared_ptr<coroutine_t> get_ptr() { return std::static_pointer_cast<coroutine_t>(shared_from_this()); };

        void init_promise() { y.promise = {}; }
    private:
        inline void call();

        static Ret invoke(std::shared_ptr<coroutine_t> coroutine, yield_t & yield, coroutine::Fn && f);

        Fn fn;
        task::execution_state state;
        std::mutex m;
        ctx::continuation continuation;
        std::atomic_bool end;
        yield_t y;
    };

    template< typename T >
    inline void coroutine<T>::call() {
        try {
            T v = invoke(get_ptr(), y, std::forward<Fn>(fn));
            y.promise.set_value(v);
        } catch (...) {
            y.promise.set_exception(std::current_exception());
        }
    }

    template< >
    inline void coroutine<void>::call() {
        try {
            invoke(get_ptr(), y, std::forward<Fn>(fn));
            y.promise.set_value();
        } catch (...) {
            y.promise.set_exception(std::current_exception());
        }
    }

    template<typename Ret>
    Ret coroutine<Ret>::invoke(std::shared_ptr<coroutine_t> coroutine, yield_t & yield, coroutine::Fn && f) {
        return f(coroutine.get(), yield);
    }


    template< typename Ret >
    class coroutine_task : public task {
    public:
        explicit coroutine_task(std::shared_ptr<coroutine<Ret>> coroutine): coro(coroutine) {
            set_execution_state(coro->get_execution_state());
        }

        ~coroutine_task() override = default;

        void execute(const execution_state & state) override {
            try {
                task::state = state;
                coro->set_execution_state(state);
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

    typedef coroutine_handler CoroutineHandler;
    typedef yield<void> YieldVoid;
}


#endif //AR_COROUTINE_H