#ifndef AR_COROUTINE_H
#define AR_COROUTINE_H

#include <iterator>
#include <type_traits>
#include <assert.h>

#include "ar/object.hpp"
#include "ar/task.hpp"
#include "ar/stack.hpp"
#include "ar/context_switcher.hpp"


namespace AsyncRuntime {
    class Task;
    class Runtime;


    class CoroutineHandler : public BaseObject {
    public:
        virtual void MakeResult() = 0;
        virtual Task* MakeExecTask() = 0;
        virtual void Suspend() = 0;
    };


    template< typename T >
    class BaseYield {
        friend Runtime;
    public:
        typedef Result<T> ResultType;


        BaseYield(CoroutineHandler*  handler) : coroutine_handler(handler) { };
        virtual ~BaseYield() =default;


        void Suspend() {
            coroutine_handler->Suspend();
        }


        void ResetResult() {
            if(!result || result->Resolved()) {
                result.reset(new ResultType());
            }
        }


        std::shared_ptr<ResultType> GetResult() const {
            return result;
        }


        void SetException(std::exception_ptr e) {
            if(result)
                result->SetException(std::current_exception());
        }


        virtual void Complete() = 0;

        CoroutineHandler*               coroutine_handler;
    protected:
        std::shared_ptr<ResultType>     result;
    };


    template< typename T >
    class Yield : public BaseYield<T> {
        typedef BaseYield<T> base;
    public:
        Yield(CoroutineHandler*  handler) : base(handler) {};
        virtual ~Yield() =default;

        void operator() (T  v) {
            value = v;

            if(base::result) {
                base::result->SetValue(value);
            }

            base::Suspend();
        };


        void Complete() override {
            if(base::result) {
                base::result->SetValue(value);
            }
        }
    private:
        T                               value;
    };


    template<>
    class Yield<void> : public BaseYield<void> {
        typedef BaseYield<void> base;
    public:
        Yield(CoroutineHandler*  handler) : base(handler) {};
        virtual ~Yield() =default;

        void operator() () {
            if(base::result) {
                base::result->SetValue();
            }

            base::Suspend();
        };


        void Complete() override {
            if(base::result) {
                base::result->SetValue();
            }
        }
    };


    typedef Yield<void> YieldVoid;


    /**
     * @class ContextRecord< StackAlloc >
     * @tparam StackAlloc
     */
    template< typename StackAlloc, class CoroutineType >
    class ContextRecord {
        typedef typename CoroutineType::YieldType                       YieldType;
        typedef std::function<void(CoroutineHandler*, YieldType&)>       Callable;
    public:
        ContextRecord(StackContext sctx, StackAlloc && salloc, Callable && fn, CoroutineType *coroutine) :
                salloc_(salloc),
                sctx_(sctx),
                fn_(fn),
                coroutine_(coroutine) { };


        ContextRecord(const ContextRecord& other) = delete;
        ContextRecord& operator =(const ContextRecord& other) = delete;
        ContextRecord(ContextRecord&& other) = delete;
        ContextRecord& operator =(ContextRecord&& other) = delete;


        fcontext_t Run(transfer_t t) {
            YieldType& yield = coroutine_->BindYieldContext(t.fctx);
            // invoke context-function
            try {
                fn_(static_cast<CoroutineHandler*>(coroutine_), yield);
                coroutine_->Complete();
            }catch (...) {
                try {
                    yield.SetException(std::current_exception());
                } catch(...) { }

                coroutine_->Complete();
            }

            return coroutine_->yield_fctx;
        }


        void Deallocate() noexcept {
            Destroy( this);
        }
    private:
        static void Destroy( ContextRecord * r) noexcept {
            typename std::decay< StackAlloc >::type salloc = std::move( r->salloc_);
            StackContext sctx = r->sctx_;
            // deallocate record
            r->~ContextRecord();
            // destroy stack with stack allocator
            salloc.Deallocate( sctx);
        }


        CoroutineType                                       *coroutine_;
        typename std::decay< StackAlloc >::type             salloc_;
        StackContext                                        sctx_;
        Callable                                            fn_;
    };


    enum CoroutineState {
        kExecuting,
        kWaiting,
    };


    template< class StackAlloc,
              class Ret >
    class BaseCoroutine: public CoroutineHandler {
        typedef BaseCoroutine<StackAlloc, Ret>                                      BaseCoroutineType;
        typedef ContextRecord< StackAlloc, BaseCoroutineType >                      Record;

        friend Record;
    public:
        typedef Yield<Ret>                                                          YieldType;
        typedef Ret                                                                 RetType;
        typedef std::function<void(CoroutineHandler*, YieldType&)>                  Callable;


        template< class Function,
                  class ...Arguments>
        explicit BaseCoroutine(Function &&fn, Arguments &&... args) :
                is_completed{false},
                yield(this),
                state{kExecuting} {
            yield.ResetResult();
            CreateRecord(std::bind( std::forward<Function>(fn),
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::forward<Arguments>(args)...) );

            fctx = Context::Jump( fctx, static_cast<void*>(record)).fctx;
        }


        BaseCoroutine(const BaseCoroutine& other) = delete;
        BaseCoroutine& operator =(const BaseCoroutine& other) = delete;
        BaseCoroutine(BaseCoroutine&& other) = delete;
        BaseCoroutine& operator =(BaseCoroutine&& other) = delete;
        virtual ~BaseCoroutine() = default;


        void operator() (const ExecutorState& executor_ = ExecutorState()) {
            std::lock_guard<std::mutex> lock(mutex);

            if(is_completed.load(std::memory_order_relaxed)) {
                throw std::runtime_error("coroutine is completed");
            }

            state.store(kExecuting, std::memory_order_relaxed);

            yield.ResetResult();

            executor = executor_;
            fctx = Context::Jump( fctx, static_cast<void*>(record)).fctx;
        }


        bool Valid() const {
            bool completed = is_completed.load(std::memory_order_relaxed);
            return !completed;
        }


        void Suspend() override {
            state.store(kWaiting, std::memory_order_relaxed);
            yield_fctx = Context::Jump( yield_fctx, nullptr).fctx;
        }


        std::shared_ptr<Result<Ret>> GetResult()  {
            return yield.GetResult();
        }


        CoroutineState GetState() const {
            CoroutineState s = state.load(std::memory_order_relaxed);
            return s;
        }


        void MakeResult() override {
            yield.ResetResult();
        }


        Task* MakeExecTask() override {
            if(!is_completed.load(std::memory_order_relaxed)) {
                auto task = MakeTask(std::bind(&BaseCoroutineType::Execute, this, std::placeholders::_1));
                task->SetDesirableExecutor(executor);
                return task;
            }else{
                return nullptr;
            }
        }
    private:
        void Execute(const ExecutorState& executor_ = ExecutorState()) {
            std::lock_guard<std::mutex> lock(mutex);

            if(is_completed.load(std::memory_order_relaxed)) {
                throw std::runtime_error("coroutine is completed");
            }

            state.store(kExecuting, std::memory_order_relaxed);
            executor = executor_;
            fctx = Context::Jump( fctx, static_cast<void*>(record)).fctx;
        }


        void CreateRecord(Callable && fn);


        Yield<Ret>& BindYieldContext( fcontext_t ctx ) {
            yield_fctx = ctx;
            return yield;
        };


        void Complete() {
            is_completed.store(true, std::memory_order_relaxed);
            yield.Complete();
        }


        ExecutorState                                       executor;
        std::atomic_bool                                    is_completed;
        std::mutex                                          mutex;
        std::atomic<CoroutineState>                         state;
        std::shared_future<void>                            future;
        Record*                                             record;
        fcontext_t                                          fctx{ nullptr };
        fcontext_t                                          yield_fctx{ nullptr };
        Yield<Ret>                                          yield;
    };


    template< typename Rec >
    transfer_t RecordExit( transfer_t t) noexcept {
        Rec * rec = static_cast< Rec * >( t.data);
        // destroy context stack
        rec->Deallocate();
        return { nullptr, nullptr };
    }


    template< typename Rec >
    void RecordEntry( transfer_t t) noexcept {
        // transfer control structure to the context-stack
        Rec * rec = static_cast< Rec * >( t.data);
        RNT_ASSERT( nullptr != t.fctx);
        RNT_ASSERT( nullptr != rec);
        try {
            t = Context::Jump( t.fctx, nullptr);
            // start executing
            t.fctx = rec->Run( t);
        }catch ( ... ) {
            std::cerr << "Exception" << std::endl;
        }
        RNT_ASSERT( nullptr !=  t.fctx);

        // destroy context-stack of `this`context on next context
        Context::OnTop(  t.fctx, rec, RecordExit< Rec >);
        RNT_ASSERT_MSG( false, "context already terminated");
    }


    template<typename StackAlloc, class Ret>
    void BaseCoroutine<StackAlloc, Ret>::CreateRecord(Callable &&fn) {
        StackAlloc salloc;
        auto sctx = salloc.Allocate();
        // reserve space for control structure
        void * storage = reinterpret_cast< void * >(
                ( reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sizeof( Record) ) )
                & ~static_cast< uintptr_t >( 0xff) );
        // placment new for control structure on context stack
        record = new ( storage) Record( sctx, std::forward< StackAlloc >( salloc), std::forward<Callable>( fn), this);
        // 64byte gab between control structure and stack top
        // should be 16byte aligned
        void * stack_top = reinterpret_cast< void * >(
                reinterpret_cast< uintptr_t >( storage) - static_cast< uintptr_t >( 64) );
        void * stack_bottom = reinterpret_cast< void * >(
                reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sctx.size) );
        // create fast-context 131072
        const std::size_t size = reinterpret_cast< uintptr_t >( stack_top) - reinterpret_cast< uintptr_t >( stack_bottom);
        fcontext_t ctx = Context::Make( stack_top, size, & RecordEntry< Record >);
        fctx = Context::Jump( ctx, record).fctx;
        RNT_ASSERT( nullptr != fctx);
    }


    template< typename Ret = void >
    class Coroutine : public BaseCoroutine<FixedSizeStack, Ret> {
        using base = BaseCoroutine<FixedSizeStack, Ret> ;
    public:
        template<   class Fn,
                    class ...Arguments>
        explicit Coroutine(Fn &&fn, Arguments &&... args) : base(std::forward<Fn>(fn), std::forward<Arguments>(args)...) { };
        virtual ~Coroutine() = default;
    };


    template< typename Ret = void, typename Fn, typename ...Arguments>
    inline Coroutine< Ret > MakeCoroutine(Fn &&fn, Arguments &&... args) {
        return Coroutine< Ret >(std::forward<Fn>(fn), std::forward<Arguments>(args)...);
    }


    template< typename Ret = void, class StackAllocator, typename Fn, typename ...Arguments>
    inline BaseCoroutine<StackAllocator, Ret> MakeCoroutine(Fn &&fn, Arguments &&... args) {
        return BaseCoroutine<StackAllocator, Ret>(std::forward<Fn>(fn), std::forward<Arguments>(args)...);
    }
}


#endif //AR_COROUTINE_H
