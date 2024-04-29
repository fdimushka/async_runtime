#ifndef AR_RUNTIME_H
#define AR_RUNTIME_H

#include "ar/work_steal_queue.hpp"
#include "ar/task.hpp"
#include "ar/notifier.hpp"
#include "ar/coroutine.hpp"
#include "ar/executor.hpp"
#include "ar/metricer.hpp"

namespace AsyncRuntime {
    class Ticker;

    struct RuntimeOptions {
        std::vector<WorkGroupOption> work_groups_option = {};
        int virtual_numa_nodes_count = 0; //for debug
    };

    class Runtime {
        friend class Processor;

        friend class Ticker;

        friend class Scheduler;

        friend class coroutine_handler;
    public:
        static Runtime *g_runtime;

        Runtime();

        ~Runtime();


        Runtime(const Runtime &) = delete;

        Runtime &operator=(const Runtime &) = delete;

        Runtime(Runtime &&) = delete;

        Runtime &operator=(Runtime &&) = delete;

        template<typename ExecutorType, class... Arguments>
        ExecutorType *CreateExecutor(Arguments &&... args);

        template<class CounterT>
        void CreateMetricer(const std::map<std::string, std::string> &labels);

        void Setup(const RuntimeOptions &options = {});

        void Setup(Runtime *other);

        void Terminate();

        template<class Callable,
                class... Arguments>
        auto Async(Callable &&f, Arguments &&... args) -> future_t<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>;

        template< typename Ret >
        future_t<Ret> Async(const std::shared_ptr<coroutine<Ret>> & coroutine);

        template<class Callable,
                class... Arguments>
        auto AsyncDelayed(Callable &&f, Timespan delay_ms,
                          Arguments &&... args) -> future_t<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>;

        template<typename ExecutorType,
                typename TaskType,
                class... Arguments>
        future_t<typename TaskType::return_type> AsyncTask(Arguments &&... args);

        template<typename ExecutorType,
                typename TaskType>
        void AsyncPostTask(std::shared_ptr<TaskType> t);

        template<typename Rep, typename Period>
        future_t<void> AsyncSleep(const std::chrono::duration<Rep, Period> &rtime);

        template<class Ret>
        Ret Await(future_t<Ret> && future);

        template<class Ret>
        Ret Await(shared_future_t<Ret> && future);

        template<class Ret>
        Ret Await(future_t<Ret> && future, CoroutineHandler *handler);

        template<class Ret>
        Ret Await(shared_future_t<Ret> && future, CoroutineHandler *handler);

        [[nodiscard]] const IExecutor *GetMainExecutor() const { return main_executor; }

        IExecutor *GetMainExecutor() { return main_executor; }

        IExecutor *GetIOExecutor() { return io_executor; }

        template<typename ExecutorType>
        const ExecutorType *GetExecutor() const {
            const std::type_info &eti = typeid(ExecutorType);
            return ((ExecutorType *) executors.at(eti.hash_code()));
        }

        template<typename ExecutorType>
        ExecutorType *GetExecutor() {
            const std::type_info &eti = typeid(ExecutorType);
            return ((ExecutorType *) executors.at(eti.hash_code()));
        }

        ObjectID GetWorkGroup(const std::string &name) const;

        EntityTag AddEntityTag(void *ptr);

        void DeleteEntityTag(EntityTag tag);

        std::shared_ptr<Mon::Counter>
        MakeMetricsCounter(const std::string &name, const std::map<std::string, std::string> &labels);

        static Runtime *Current() { return g_runtime; }

    protected:
        std::shared_ptr<Mon::Counter> coroutine_counter;
    private:
        void SetupWorkGroups(const std::vector<WorkGroupOption> &work_groups_option);

        void CheckRuntime();

        void Post(const std::shared_ptr<task> & task);

        void CreateDefaultExecutors(int virtual_numa_nodes_count = 0);

        void CreateTbbExecutors();

        IExecutor *FetchExecutor(ExecutorType type, const EntityTag &tag);

        IExecutor *FetchFreeExecutor(ExecutorType type);

        std::vector<WorkGroupOption> work_groups_option;
        std::map<size_t, IExecutor *> executors;
        IExecutor *main_executor;
        IExecutor *io_executor;
        bool is_setup;
        std::unique_ptr<Mon::IMetricer> metricer;
    };

    template<class Callable, class... Arguments>
    auto Runtime::Async(Callable &&f, Arguments &&... args)
    -> future_t<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))> {
        CheckRuntime();
        auto task = make_task_shared_ptr(std::bind(std::forward<Callable>(f), std::forward<Arguments>(args)...));
        Post(task);
        return task->get_future();
    }

    template<class Callable, class... Arguments>
    auto Runtime::AsyncDelayed(Callable &&f, Timespan delay_ms, Arguments &&... args)
    -> future_t<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))> {
        CheckRuntime();
        auto task = make_task_shared_ptr(std::bind(std::forward<Callable>(f), std::forward<Arguments>(args)...));
        task->template set_delay<Timestamp::Milli>(delay_ms);
        Post(task);
        return task->get_future();
    }

    template< typename Ret >
    future_t<Ret> Runtime::Async(const std::shared_ptr<coroutine<Ret>> & coroutine) {
        CheckRuntime();
        coroutine->init_promise();
        auto task = std::make_shared<coroutine_task<Ret>>(coroutine);
        Post(task);
        return task->get_future();
    }

    template<typename Rep, typename Period>
    future_t<void> Runtime::AsyncSleep(const std::chrono::duration<Rep, Period> &rtime) {
        CheckRuntime();
        auto task = make_dummy_task_shared_ptr();
        task->set_delay<std::chrono::duration<Rep, Period> >(rtime.count());
        Post(task);
        return task->get_future();
    }

    template<typename ExecutorType, typename TaskType, class... Arguments>
    future_t<typename TaskType::return_type> Runtime::AsyncTask(Arguments &&... args) {
        static_assert(std::is_base_of<task, TaskType>::value, "TaskType must derive from Task");
        CheckRuntime();
        auto task = std::make_shared<TaskType>(std::forward<Arguments>(args)...);
        const std::type_info &eti = typeid(ExecutorType);
        ((ExecutorType *) executors.at(eti.hash_code()))->Post(task);
        return task->get_future();
    }

    template<typename ExecutorType, typename TaskType>
    void Runtime::AsyncPostTask(std::shared_ptr<TaskType> t) {
        static_assert(std::is_base_of<task, TaskType>::value, "TaskType must derive from Task");
        CheckRuntime();
        const std::type_info &eti = typeid(ExecutorType);
        ((ExecutorType *) executors.at(eti.hash_code()))->Post(t);
    }

    template<class Ret>
    Ret Runtime::Await(future_t<Ret> && future) {
        CheckRuntime();
        future.wait();
        return future.get();
    }

    template<class Ret>
    Ret Runtime::Await(shared_future_t<Ret> && future) {
        CheckRuntime();
        future.wait();
        return future.get();
    }

    template< class Ret >
    Ret Runtime::Await(future_t<Ret> && future, coroutine_handler *handler) {
        if (!future.has_value() && !future.has_exception()) {
            future_t<Ret> done_future;
            handler->suspend_with([&future, &done_future](coroutine_handler *handler) {
                done_future = future.then(boost::launch::sync, [handler](future_t<Ret> f) {
                    auto task = handler->resume_task();
                    Runtime::g_runtime->Post(task);
                    return f.get();
                });
            });
            return done_future.get();
        } else {
            return future.get();
        }
    }

    template< class Ret >
    Ret Runtime::Await(shared_future_t<Ret> && future, coroutine_handler *handler) {
        if (!future.has_value() && !future.has_exception()) {
            future_t<Ret> done_future;
            handler->suspend_with([&future, &done_future](coroutine_handler *handler) {
                done_future = future.then(boost::launch::sync, [handler](future_t<Ret> f) {
                    auto task = handler->resume_task();
                    Runtime::g_runtime->Post(task);
                    return f.get();
                });
            });
            return done_future.get();
        } else {
            return future.get();
        }
    }

    template<typename ExecutorType,
            class... Arguments>
    ExecutorType *Runtime::CreateExecutor(Arguments &&... args) {
        static_assert(std::is_base_of<IExecutor, ExecutorType>::value, "ExecutorType must derive from IExecutor");
        const std::type_info &eti = typeid(ExecutorType);
        auto *executor = new ExecutorType(std::forward<Arguments>(args)...);
        executors.insert(std::make_pair(eti.hash_code(), executor));
        return executor;
    }

    template<class CounterT>
    void Runtime::CreateMetricer(const std::map<std::string, std::string> &labels) {
        metricer = std::make_unique<Mon::Metricer<CounterT>>(labels);
    }

    inline void SetupRuntime(const RuntimeOptions &options = {}) {
        Runtime::g_runtime = new Runtime;
        Runtime::g_runtime->Setup(options);
    }

    inline void SetupRuntime(Runtime *other) {
        Runtime::g_runtime->Setup(other);
    }

    inline void Terminate() {
        if (Runtime::g_runtime != nullptr) {
            Runtime::g_runtime->Terminate();
            delete Runtime::g_runtime;
            Runtime::g_runtime = nullptr;
        }
    }

    template<typename ExecutorType,
            class... Arguments>
    inline ExecutorType *CreateExecutor(Arguments &&... args) {
        return Runtime::g_runtime->CreateExecutor<ExecutorType>(std::forward<Arguments>(args)...);
    }

    template<class CounterT>
    inline void CreateMetricer(const std::map<std::string, std::string> &labels) {
        Runtime::g_runtime->CreateMetricer<CounterT>(labels);
    }

    inline ObjectID GetWorkGroup(const std::string &name) {
        return Runtime::g_runtime->GetWorkGroup(name);
    }

    inline EntityTag AddEntityTag(void *ptr) {
        return Runtime::g_runtime->AddEntityTag(ptr);
    }

    inline EntityTag AddEntityTag(CoroutineHandler* handler, void *ptr) {
        return Runtime::g_runtime->AddEntityTag(ptr);
    }

    inline void DeleteEntityTag(EntityTag tag) {
        return Runtime::g_runtime->DeleteEntityTag(tag);
    }

    template<class Callable,
            class... Arguments>
    inline auto Async(Callable &&f, Arguments &&... args) -> future_t<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))> {
        return Runtime::g_runtime->Async(std::forward<Callable>(f), std::forward<Arguments>(args)...);
    }


    /**
     * @brief async call
     * @tparam CoroutineType
     */
    template<class Ret>
    inline future_t<Ret> Async(const std::shared_ptr<coroutine<Ret>> & coroutine) {
        return Runtime::g_runtime->Async(coroutine);
    }

    /**
     * @brief await
     * @tparam Ret
     * @param awaiter
     * @param context
     * @return
     */
    template<class Ret>
    inline Ret Await(future_t<Ret> && future) {
        return Runtime::g_runtime->Await(std::forward<future_t<Ret>>(future));
    }

    /**
     * @brief
     * @tparam Ret
     * @param future
     * @return
     */
    template<class Ret>
    inline Ret Await(shared_future_t<Ret> && future) {
        return Runtime::g_runtime->Await(std::forward<shared_future_t<Ret>>(future));
    }

    /**
     * @brief await
     * @tparam Ret
     * @param awaiter
     * @param context
     * @return
     */
    template<class Ret>
    inline Ret Await(future_t<Ret> && future, CoroutineHandler *handler) {
        return Runtime::g_runtime->Await<Ret>(std::forward<future_t<Ret>>(future), handler);
    }

    /**
     * @brief
     * @tparam Ret
     * @param future
     * @param handler
     * @return
     */
    template<class Ret>
    inline Ret Await(shared_future_t<Ret> && future, CoroutineHandler *handler) {
        return Runtime::g_runtime->Await<Ret>(std::forward<shared_future_t<Ret>>(future), handler);
    }


/**
     * @brief
     * @tparam ExecutorType
     * @tparam TaskType
     * @param task
     */
    template<typename ExecutorType, typename TaskType>
    inline void AsyncPostTask(std::shared_ptr<TaskType> t) {
        return Runtime::g_runtime->AsyncPostTask<ExecutorType, TaskType>(t);
    }

    /**
     * @brief async sleep
     * @tparam _Rep
     * @tparam _Period
     * @param rtime
     */
    template<typename Rep, typename Period>
    inline future_t<void> AsyncSleep(const std::chrono::duration<Rep, Period> &rtime) {
        return Runtime::g_runtime->AsyncSleep<Rep, Period>(rtime);
    }


    /**
     * @brief async open file
     * @param stream
     * @param filename
     * @param flags
     * @param mods
     * @return
     */
//    inline future_t<IOResult> AsyncFsOpen(const char *filename, int flags = O_RDWR | O_CREAT, int mode = S_IRWXU) {
//        return Runtime::g_runtime->AsyncIO<FsOpenTask>(filename, flags, mode);
//    }
//
//
//    /**
//     * @brief async close file
//     * @param stream
//     * @return
//     */
//    inline future_t<IOResult> AsyncFsClose(int fd) {
//        return Runtime::g_runtime->AsyncIO<FsCloseTask>(fd);
//    }
//
//
//    /**
//     * @brief async read from file
//     * @param stream
//     * @param offset
//     * @return
//     */
//    inline future_t<IOResult> AsyncFsRead(int fd, const IOStreamPtr &stream, int64_t seek = -1, int64_t size = -1) {
//        return Runtime::g_runtime->AsyncIO<FsReadTask>(fd, stream, seek);
//    }
//
//
//    /**
//     * @brief async write to file
//     * @param stream
//     * @param offset
//     * @return
//     */
//    inline future_t<IOResult> AsyncFsWrite(int fd, const IOStreamPtr &stream, int64_t seek = -1) {
//        return Runtime::g_runtime->AsyncIO<FsWriteTask>(fd, stream, seek);
//    }
//
//
//    /**
//     * @brief
//     * @param handle_connection
//     * @return
//     */
//    inline future_t<IOResult> AsyncConnect(const TCPConnectionPtr &connection) {
//        return Runtime::g_runtime->AsyncIO<NetConnectionTask>(connection);
//    }
//
//
//    /**
//     * @brief
//     * @param server
//     * @param callback
//     * @return
//     */
//    inline future_t<IOResult> AsyncListen(const TCPServerPtr &server, const TCPSession::CallbackType &callback) {
//        return Runtime::g_runtime->AsyncIO<NetListenTask>(server, callback);
//    }
//
//
//    /**
//     * @brief
//     * @param connection
//     * @param stream
//     * @return
//     */
//    inline future_t<IOResult>
//    AsyncRead(CoroutineHandler *handler, const TCPConnectionPtr &connection, char *buffer, size_t size) {
////        while (1) {
////            if (connection->read_error.load(std::memory_order_relaxed) < 0) { break; }
////            int res = Await(AsyncWaitReadStream(connection, size), handler);
////            if (res >= size) { break; }
////        }
////        int res_size = ConsumeReadStream(connection, buffer, size);
////        return std::make_shared<AsyncRuntime::Result<int>>(res_size);
//        return {};
//    }
//
//
//    /**
//     * @brief
//     * @param connection
//     * @param stream
//     * @return
//     */
//    inline future_t<IOResult> AsyncWrite(const TCPConnectionPtr &connection, const char *buffer, size_t size) {
//        return Runtime::g_runtime->AsyncIO<NetWriteTask>(connection, buffer, size);
//    }
//
//    /**
//     * @brief
//     * @param connection
//     * @param buffer
//     * @param size
//     * @return
//     */
//    inline future_t<IOResult> AsyncSend(const TCPConnectionPtr &connection, const char *buffer, size_t size) {
//        return Runtime::g_runtime->AsyncIO<NetTCPSendTask>(connection, buffer, size);
//    }
//
//
//    /**
//     * @brief
//     * @param connection
//     * @return
//     */
//    inline future_t<IOResult> AsyncClose(const TCPConnectionPtr &connection) {
//        return Runtime::g_runtime->AsyncIO<NetCloseTask>(connection);
//    }
//
//
//    /**
//     * @brief
//     * @param info
//     * @return
//     */
//    inline future_t<IOResult> AsyncNetAddrInfo(const NetAddrInfoPtr &info) {
//        return Runtime::g_runtime->AsyncIO<NetAddrInfoTask>(info);
//    }
//
//
//    /**
//     * @brief async bind udp socket
//     * @param udp
//     * @param broadcast
//     * @return
//     */
//    inline future_t<IOResult> AsyncUDPBind(const UDPPtr &udp, int flags = 0, bool broadcast = false) {
//        return Runtime::g_runtime->AsyncIO<NetUDPBindTask>(udp, flags, broadcast);
//    }
//
//
//    /**
//     * @brief async send to address
//     * @param udp
//     * @param stream
//     * @param send_addr
//     * @return
//     */
//    inline future_t<IOResult> AsyncSend(const UDPPtr &udp, const IOStreamPtr &stream, const IPv4Addr &send_addr) {
//        return Runtime::g_runtime->AsyncIO<NetSendTask>(udp, stream, send_addr);
//    }
//
//
//    /**
//     * @brief
//     * @param udp
//     * @param stream
//     * @return
//     */
//    inline future_t<IOResult> AsyncRecv(const UDPPtr &udp, const IOStreamPtr &stream) {
//        return Runtime::g_runtime->AsyncIO<NetRecvTask>(udp, stream);
//    }
}


#endif //AR_RUNTIME_H
