#ifndef AR_DATAFLOW_KERNEL_H
#define AR_DATAFLOW_KERNEL_H

#include "ar/ar.hpp"
#include "ar/dataflow/sink.hpp"
#include "ar/dataflow/source.hpp"
#include "ar/dataflow/notifier.hpp"
#include "ar/dataflow/port.hpp"
#include "ar/dataflow/kernel_events.hpp"
#include "ar/allocators.hpp"

namespace AsyncRuntime::Dataflow {

    enum KernelProcessResult : int {
        kNEXT  =0,
        kEND   =1,
        kERROR =2
    };

    enum KernelState : int {
        kREADY = 0,
        kINITIALIZED = 1,
        kRUNNING = 2,
        kTERMINATED = 3
    };

    /**
     * @brief
     */
    class KernelContext {
    public:
        KernelContext() = default;
        virtual ~KernelContext() = default;

        void SetErrorCode(int code) { error_code = code; }
        void SetInterruptCallback(const std::function<int(void*)> & callback, void *opaque) {
            interrupt_callback = callback;
            interrupt_callback_opaque = opaque;
        }

        int GetErrorCode() const { return error_code; }

        void SetResource(resource_pool *res) { resource = res; }

        resource_pool *GetResource() { return resource; }
    protected:
        int Interrupt() {
            if (interrupt_callback) {
                return interrupt_callback(interrupt_callback_opaque);
            } else {
                return 0;
            }
        }

      resource_pool *resource = nullptr;

    private:
        std::function<int(void*)> interrupt_callback;
        void *interrupt_callback_opaque = nullptr;
        int error_code = 0;
    };

    /**
     * @brief
     * @tparam KernelContextT
     */
    template< class KernelContextT >
    class Kernel {
        static_assert(std::is_base_of<KernelContext, KernelContextT>::value,
                      "KernelContextT must derive from KernelContext");
    public:
        explicit Kernel(const std::string &name);
        Kernel(resource_pool *resource, const std::string &name);

        virtual ~Kernel();

        AsyncRuntime::future_t<int> AsyncInit();

        bool Run(const std::function<void(int)> &terminated_callback);
        bool Run();

        AsyncRuntime::shared_future_t<int> AsyncTerminate();

        void SetWorkGroup(AsyncRuntime::ObjectID wg) { coroutine->set_execution_state_wg(wg); }

        void SetEntityTag(AsyncRuntime::EntityTag tag) { coroutine->set_execution_state_tag(tag); }

        const std::string &GetName() const { return name; }

        const Source &GetSource() const { return source; }

        const Sink &GetSink() const { return sink; }

        Sink &GetSink() { return sink; }

        template<class T>
        std::shared_ptr<SourcePort<T>> GetSourcePort( const std::string & name ) { return source.template At<T>(name); };

        template<class T>
        std::shared_ptr<SinkPort<T>> GetSinkPort( const std::string & name ) { return sink.template At<T>(name); }

        size_t GetCpuTime() {
            return coroutine->GetCpuTime();
        }
    protected:
        virtual int OnInit(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) = 0;

        virtual KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) = 0;

        virtual KernelProcessResult OnSinkSubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual KernelProcessResult OnWaitSinkSubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual KernelProcessResult OnSinkUnsubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual KernelProcessResult OnTerminate(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual KernelProcessResult OnUpdate(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual void OnDispose(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) { };

        static int AsyncLoop(AsyncRuntime::CoroutineHandler *handler, yield<int> &yield, Kernel<KernelContextT>* kernel);

        Source source;
        Sink sink;
        Notifier process_notifier;
        resource_pool *resource = nullptr;
    private:
        std::atomic<KernelState> state;
        std::string name;
        shared_future_t<int> future_res;
        std::shared_ptr<AsyncRuntime::coroutine<int>> coroutine;
    };

    template<class KernelContextT>
    Kernel<KernelContextT>::Kernel(const std::string &name)
            : source(&process_notifier)
            , sink(&process_notifier)
            , name(name)
            , state{kREADY} {
    }

    template<class KernelContextT>
    Kernel<KernelContextT>::Kernel(resource_pool *res, const std::string &name)
            : source(res, &process_notifier)
            , sink(&process_notifier)
            , name(name)
            , state{kREADY}
            , resource(res) {
    }

    template<class KernelContextT>
    Kernel<KernelContextT>::~Kernel() {
        source.Flush();
        sink.DisconnectAll();
    }

    template<class KernelContextT>
    int Kernel<KernelContextT>::AsyncLoop(AsyncRuntime::CoroutineHandler *handler,
                                          yield<int> &yield,
                                          Kernel<KernelContextT>* kernel) {
        KernelContextT context;
        context.SetResource(kernel->resource);
        try {
            int init_error = kernel->OnInit(handler, &context);
            if (init_error != 0) {
                kernel->OnDispose(handler, &context);
                kernel->state.store(kTERMINATED, std::memory_order_relaxed);
                return init_error;
            } else {
                kernel->state.store(kINITIALIZED, std::memory_order_relaxed);
            }

            yield(init_error);
            if (kernel->state.load(std::memory_order_relaxed) != kTERMINATED) {
                kernel->state.store(kRUNNING, std::memory_order_relaxed);

                auto res = kNEXT;
                while (res == kNEXT && kernel->state.load(std::memory_order_relaxed) != kTERMINATED) {
                    res = kernel->OnUpdate(handler, &context);
                }
            }

            kernel->state.store(kTERMINATED, std::memory_order_relaxed);
            kernel->OnDispose(handler, &context);
        } catch (std::exception & ex) {
            std::cerr << ex.what() << std::endl;
            kernel->state.store(kTERMINATED, std::memory_order_relaxed);
            kernel->OnDispose(handler, &context);
            return -1;
        }

        return context.GetErrorCode();
    }

    template<class KernelContextT>
    AsyncRuntime::future_t<int> Kernel<KernelContextT>::AsyncInit() {
        try {
            if (resource != nullptr) {
                coroutine = make_coroutine<int>(resource, &Kernel<KernelContextT>::AsyncLoop, this);
            }else{
                coroutine = make_coroutine<int>(&Kernel<KernelContextT>::AsyncLoop, this);
            }
            state.store(kREADY, std::memory_order_relaxed);
            return AsyncRuntime::Async(coroutine);
        } catch (...) {
            return make_resolved_future(-1);
        }
    }

    template<class KernelContextT>
    bool Kernel<KernelContextT>::Run() {
        if (state.load(std::memory_order_relaxed) != kINITIALIZED) {
            return false;
        }

        if (coroutine && !coroutine->is_completed()) {
            state.store(kRUNNING, std::memory_order_relaxed);
            auto f = AsyncRuntime::Async(coroutine);
            future_res = f.share();
            return true;
        } else {
            return false;
        }
    }

    template<class KernelContextT>
    bool Kernel<KernelContextT>::Run(const std::function<void(int)> &terminated_callback) {
        if (state.load(std::memory_order_relaxed) != kINITIALIZED) {
            return false;
        }
        if (coroutine && !coroutine->is_completed()) {
            state.store(kRUNNING, std::memory_order_relaxed);
            auto f = AsyncRuntime::Async(coroutine);
            future_res = f.share();

            if (terminated_callback) {
                future_res.then(boost::launch::sync, [terminated_callback](boost::shared_future<int> f) {
                    int res = f.get();
                    if (terminated_callback) {
                        terminated_callback(res);
                    }
                    return res;
                });
            }
            return true;
        } else {
            return false;
        }
    }

    template<class KernelContextT>
    AsyncRuntime::shared_future_t<int> Kernel<KernelContextT>::AsyncTerminate() {
        if (state.load(std::memory_order_relaxed) == kRUNNING) {
            state.store(kTERMINATED, std::memory_order_relaxed);
            process_notifier.Notify((int) KernelEvent::kKERNEL_EVENT_TERMINATE);
            return future_res;
        } else if (state.load(std::memory_order_relaxed) == kINITIALIZED) {
            state.store(kTERMINATED, std::memory_order_relaxed);
            process_notifier.Notify((int) KernelEvent::kKERNEL_EVENT_TERMINATE);
            auto f = AsyncRuntime::Async(coroutine);
            future_res = f.share();
            return future_res;
        } else {
            state.store(kTERMINATED, std::memory_order_relaxed);
            return make_resolved_future(0);
        }
    }

    template<class KernelContextT>
    KernelProcessResult
    Kernel<KernelContextT>::OnUpdate(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) {
        KernelProcessResult res = kNEXT;
        if (sink.SubscribersEmpty()) {
            res = OnWaitSinkSubscription(handler, context);
            if (res != kNEXT) {
                return res;
            }
        }

        int events = AsyncRuntime::Await(process_notifier.AsyncWatchAny(), handler);

        if (Dataflow::Notifier::HasState(events, KernelEvent::kKERNEL_EVENT_READ_SOURCE)) {
            res = OnProcess(handler, context);
            if (res != kNEXT) {
                return res;
            }
        }

        if (Dataflow::Notifier::HasState(events, KernelEvent::kKERNEL_EVENT_TERMINATE)) {
            res = OnTerminate(handler, context);
            if (res != kNEXT) {
                return res;
            }
        }

        if (Dataflow::Notifier::HasState(events, KernelEvent::kKERNEL_EVENT_SINK_SUBSCRIPTION)) {
            res = OnSinkSubscription(handler, context);
            if (res != kNEXT) {
                return res;
            }
        }

        if (Dataflow::Notifier::HasState(events, KernelEvent::kKERNEL_EVENT_SINK_UNSUBSCRIPTION)) {
            res = OnSinkUnsubscription(handler, context);
            if (res != kNEXT) {
                return res;
            }
        }

        return res;
    }

    template<class KernelContextT>
    KernelProcessResult
    Kernel<KernelContextT>::OnTerminate(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) {
        source.Deactivate();
        source.Flush();
        sink.UnsubscribeAll();
        return kEND;
    }

    template<class KernelContextT>
    KernelProcessResult
    Kernel<KernelContextT>::OnSinkSubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) {
        source.Activate();
        return kNEXT;
    }

    template<class KernelContextT>
    KernelProcessResult
    Kernel<KernelContextT>::OnSinkUnsubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) {
        KernelProcessResult res = kNEXT;
        if (sink.SubscribersEmpty()) {
            res = OnWaitSinkSubscription(handler, context);
        }

        return res;
    }

    template<class KernelContextT>
    KernelProcessResult
    Kernel<KernelContextT>::OnWaitSinkSubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) {
        source.Flush();
        source.Deactivate();

        KernelProcessResult res = kNEXT;
        int events = AsyncRuntime::Await(process_notifier.AsyncWatch(KernelEvent::kKERNEL_EVENT_SINK_SUBSCRIPTION, KernelEvent::kKERNEL_EVENT_TERMINATE), handler);

        if (Dataflow::Notifier::HasState(events, KernelEvent::kKERNEL_EVENT_TERMINATE)) {
            res = OnTerminate(handler, context);
            if (res != kNEXT) {
                return res;
            }
        }

        if (Dataflow::Notifier::HasState(events, KernelEvent::kKERNEL_EVENT_SINK_SUBSCRIPTION)) {
            res = OnSinkSubscription(handler, context);
            source.Flush();
            if (res != kNEXT) {
                return res;
            }
        }

        return res;
    }
}

#endif //AR_DATAFLOW_KERNEL_H
