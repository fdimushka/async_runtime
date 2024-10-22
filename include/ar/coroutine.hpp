#ifndef AR_COROUTINE_H
#define AR_COROUTINE_H

#include <iterator>
#include <type_traits>

#include "ar/task.hpp"
#include "ar/resource_pool.hpp"
#include "ar/stack.hpp"
#include "ar/pooled_stack.hpp"
#include "ar/allocators.hpp"

namespace AsyncRuntime {
    inline resource_pool * GetResource();
    inline resource_pool * GetResource(int64_t resource_id);
    inline resource_pool * GetCurrentResource();

    namespace ctx = boost::context;

    template< typename T >
    class coroutine;

    template< typename T >
    class coroutine_task;

    void set_current_resource(resource_pool *resource);

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

        virtual resource_pool *get_resource() const = 0;

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
    public:
        coroutine() = default;

        explicit coroutine(coroutine::Fn &&f) : fn(f), end{false}, resource{nullptr} {
            continuation = ctx::continuation(ctx::callcc(std::allocator_arg, basic_fixedsize_stack<ctx::stack_traits>(), [this](ctx::continuation &&c) {
                y.continuation = std::move(c);
                y.continuation = y.continuation.resume();
                call();
                end.store(true, std::memory_order_relaxed);
                return std::move(y.continuation);
            }));
        }

        explicit coroutine(coroutine::Fn &&f, resource_pool *res) : fn(f), end{false}, resource{res} {
            continuation = ctx::continuation(ctx::callcc(std::allocator_arg, pooled_fixedsize_stack<ctx::stack_traits>(res), [this](ctx::continuation &&c) {
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

        resource_pool *get_resource() const final { return resource; }

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
        resource_pool *resource = nullptr;
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
                set_current_resource(coro->get_resource());
                task::state = state;
                coro->set_execution_state(state);
                coro->resume();
                set_current_resource(nullptr);
            } catch (std::exception & ex) {
                std::cerr << ex.what() << std::endl;
                set_current_resource(nullptr);
            }
        }

        future_t<Ret> get_future() { return coro->get_future(); }
    private:
        std::shared_ptr<coroutine<Ret>> coro;
    };

    template<typename T>
    Allocator<T>::Allocator(const coroutine_handler *handler) {
        if (handler->get_resource() != nullptr) {
            resource = handler->get_resource();
        } else {
            resource = GetResource();
        }
    }

    template <typename Ret = void, typename Fn, typename ...Arguments>
    std::shared_ptr<coroutine<Ret>> make_coroutine(Fn &&fn, Arguments &&... args) {
        return std::make_shared<coroutine<Ret>>(std::bind(std::forward<Fn>(fn), std::placeholders::_1, std::placeholders::_2, std::forward<Arguments>(args)...));
    }

    template <typename Ret = void, typename Fn>
    std::shared_ptr<coroutine<Ret>> make_coroutine(Fn &&fn) {
        return std::make_shared<coroutine<Ret>>(std::forward<Fn>(fn));
    }

    template <typename Ret = void, typename Fn, typename ...Arguments>
    std::shared_ptr<coroutine<Ret>> make_coroutine(int64_t resource_id, Fn &&fn, Arguments &&... args) {
        auto resource = GetResource(resource_id);
        return std::allocate_shared<coroutine<Ret>>(Allocator<coroutine<Ret>>{resource},
                std::bind(std::forward<Fn>(fn), std::placeholders::_1, std::placeholders::_2, std::forward<Arguments>(args)...),
                resource);
    }

    template <typename Ret = void, typename Fn>
    std::shared_ptr<coroutine<Ret>> make_coroutine(int64_t resource_id, Fn &&fn) {
        auto resource = GetResource(resource_id);
        return std::allocate_shared<coroutine<Ret>>(Allocator<coroutine<Ret>>{resource}, std::forward<Fn>(fn), resource);
    }

    template <typename Ret = void, typename Fn, typename ...Arguments>
    std::shared_ptr<coroutine<Ret>> make_coroutine(resource_pool *resource, Fn &&fn, Arguments &&... args) {
        return std::allocate_shared<coroutine<Ret>>(Allocator<coroutine<Ret>>{resource},
                std::bind(std::forward<Fn>(fn), std::placeholders::_1, std::placeholders::_2, std::forward<Arguments>(args)...),
                resource);
    }

    template <typename Ret = void, typename Fn>
    std::shared_ptr<coroutine<Ret>> make_coroutine(resource_pool *resource, Fn &&fn) {
        return std::allocate_shared<coroutine<Ret>>(Allocator<coroutine<Ret>>{resource}, std::forward<Fn>(fn), resource);
    }

    typedef coroutine_handler CoroutineHandler;
    typedef yield<void> YieldVoid;
}


#endif //AR_COROUTINE_H