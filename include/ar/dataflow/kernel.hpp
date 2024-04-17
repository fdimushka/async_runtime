#ifndef AR_DATAFLOW_KERNEL_H
#define AR_DATAFLOW_KERNEL_H

#include "ar/ar.hpp"
#include "ar/dataflow/sink.hpp"
#include "ar/dataflow/source.hpp"
#include "ar/dataflow/notifier.hpp"
#include "ar/dataflow/port.hpp"
#include "ar/dataflow/kernel_events.hpp"

namespace AsyncRuntime::Dataflow {

    enum KernelProcessResult : int {
        kNEXT  =0,
        kEND   =1,
        kERROR =2
    };

    /**
     * @brief
     */
    class KernelContext {
    public:
        KernelContext() = default;
        virtual ~KernelContext() = default;
        //AsyncRuntime::CoroutineHandler *coroutine_handler_ = nullptr;
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

        virtual ~Kernel();

        void Run();
        std::shared_ptr<AsyncRuntime::Result<int>> AsyncRun();

        void SetWorkGroup(AsyncRuntime::ObjectID wg) { coroutine.SetWorkGroup(wg);}

        void SetEntityTag(AsyncRuntime::EntityTag tag) { coroutine.SetEntityTag(tag);}

        void Terminate();

        AsyncRuntime::ResultVoidPtr AsyncTerminate();

        const std::string &GetName() const { return name; }

        const Source &GetSource() const { return source; }

        const Sink &GetSink() const { return sink; }
        Sink &GetSink() { return sink; }

        template<class T>
        std::shared_ptr<SourcePort<T>> GetSourcePort( const std::string & name ) { return source.template At<T>(name); };

        template<class T>
        std::shared_ptr<SinkPort<T>> GetSinkPort( const std::string & name ) { return sink.template At<T>(name); }
    protected:
        virtual int OnInit(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) = 0;

        virtual KernelProcessResult OnProcess(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) = 0;

        virtual KernelProcessResult OnSinkSubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual KernelProcessResult OnWaitSinkSubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual KernelProcessResult OnSinkUnsubscription(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual KernelProcessResult OnTerminate(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual KernelProcessResult OnUpdate(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context);

        virtual void OnDispose(AsyncRuntime::CoroutineHandler *handler, KernelContextT *context) { };

        static void AsyncLoop(AsyncRuntime::CoroutineHandler *handler, AsyncRuntime::YieldVoid &yield, Kernel<KernelContextT> *kernel);

        Source source;
        Sink sink;
        Notifier process_notifier;
    private:
        std::string name;
        std::shared_ptr<AsyncRuntime::Result<int>> init_result;
        AsyncRuntime::ResultVoidPtr coroutine_result;
        AsyncRuntime::Coroutine<void> coroutine;
    };

    template<class KernelContextT>
    Kernel<KernelContextT>::Kernel(const std::string &name)
            : source(&process_notifier)
            , sink(&process_notifier)
            , name(name)
            , init_result(std::make_shared<AsyncRuntime::Result<int>>())
            , coroutine(&Kernel<KernelContextT>::AsyncLoop, this) { }

    template<class KernelContextT>
    Kernel<KernelContextT>::~Kernel() {
        source.Flush();
        sink.DisconnectAll();
        Terminate();
    }

    template<class KernelContextT>
    void Kernel<KernelContextT>::AsyncLoop(AsyncRuntime::CoroutineHandler *handler,
                                          AsyncRuntime::YieldVoid &yield,
                                          Kernel<KernelContextT> *kernel) {
        KernelContextT context;
        yield();
        try {
            int init_error = kernel->OnInit(handler, &context);
            if (init_error == 0) {
                kernel->init_result->SetValue(init_error);
            } else {
                kernel->OnDispose(handler, &context);
                kernel->init_result->SetValue(init_error);
                return;
            }
            auto res = kNEXT;
            while ( res == kNEXT  ) {
                res = kernel->OnUpdate(handler, &context);
            }

            kernel->OnDispose(handler, &context);
        } catch (std::exception & ex) {
            std::cerr << ex.what() << std::endl;
            kernel->OnDispose(handler, &context);
        }
    }

    template<class KernelContextT>
    void Kernel<KernelContextT>::Run() {
        coroutine_result = AsyncRuntime::Async(coroutine);
    }

    template<class KernelContextT>
    std::shared_ptr<AsyncRuntime::Result<int>> Kernel<KernelContextT>::AsyncRun() {
        coroutine_result = AsyncRuntime::Async(coroutine);
        return init_result;
    }

    template<class KernelContextT>
    void Kernel<KernelContextT>::Terminate() {
        process_notifier.Notify((int)KernelEvent::kKERNEL_EVENT_TERMINATE);
        if (coroutine_result) {
            coroutine_result->Wait();
        }
    }

    template<class KernelContextT>
    AsyncRuntime::ResultVoidPtr Kernel<KernelContextT>::AsyncTerminate() {
        process_notifier.Notify((int)KernelEvent::kKERNEL_EVENT_TERMINATE);
        return coroutine_result;
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

        int events = AsyncRuntime::Await(process_notifier.AsyncWatchAll(), handler);

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
