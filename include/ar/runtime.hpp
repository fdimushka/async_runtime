#ifndef AR_RUNTIME_H
#define AR_RUNTIME_H

#include "ar/work_steal_queue.hpp"
#include "ar/task.hpp"
#include "ar/notifier.hpp"
#include "ar/coroutine.hpp"
#include "ar/awaiter.hpp"
#include "ar/channel.hpp"
#include "ar/executor.hpp"
#include "ar/io_task.hpp"
#include "ar/io_executor.hpp"
#include "ar/metricer.hpp"
#include "ar/stream.hpp"


namespace AsyncRuntime {
    class Ticker;


    struct RuntimeOptions {
        std::vector<WorkGroupOption> work_groups_option = {};
        int virtual_numa_nodes_count = 0; //for debug
    };


    /**
     * @class Runtime
     * @brief runtime class
     */
    class Runtime {
        friend class Processor;
        friend class Ticker;
        friend class CoroutineHandler;
    public:
        static Runtime g_runtime;

        Runtime();
        ~Runtime();


        Runtime(const Runtime&) = delete;
        Runtime& operator =(const Runtime&) = delete;
        Runtime(Runtime&&) = delete;
        Runtime& operator =(Runtime&&) = delete;

        /**
         * @brief
         * @param executor
         */
        template < typename ExecutorType,
                class... Arguments >
        ExecutorType* CreateExecutor(Arguments&&... args);

        /**
         * @brief
         * @tparam MetricerT
         */
        template< class CounterT >
        void CreateMetricer(const std::map<std::string, std::string> &labels);


        /**
         * @brief
         */
        void Setup(const RuntimeOptions& options = {});
        void Terminate();


        /**
         * @brief
         * @tparam Callable
         * @tparam Arguments
         * @param f
         * @param args
         * @return
         */
        template <  class Callable,
                class... Arguments >
        auto Async(Callable&& f, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>>;


        /**
         * @brief
         * @tparam Callable
         * @tparam Arguments
         * @param f
         * @param delay_ms
         * @param args
         * @return
         */
        template <  class Callable,
                class... Arguments >
        auto AsyncDelayed(Callable&& f, Timespan delay_ms, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>>;



        /**
         * @brief async call
         * @tparam CoroutineType
         */
        template< class CoroutineType >
        std::shared_ptr<Result<typename CoroutineType::RetType>> Async(CoroutineType & coroutine);


        /**
         * @brief
         * @return
         */
        template< typename ExecutorType,
                typename TaskType,
                class... Arguments >
        inline std::shared_ptr<Result<typename TaskType::return_type>> AsyncTask(Arguments&&... args);


        /**
         * @brief
         * @tparam ExecutorType
         * @tparam TaskType
         * @param task
         */
        template< typename ExecutorType,
                typename TaskType >
        inline void AsyncPostTask(TaskType *task);


        /**
         * @brief
         * @return
         */
        template< typename IOTaskType,
                class... Arguments >
        std::shared_ptr<Result<IOResult>> AsyncIO(Arguments&&... args);



        /**
         * @brief
         * @tparam Rep
         * @tparam Period
         * @param rtime
         * @return
         */
        template< typename Rep, typename Period >
        inline ResultVoidPtr AsyncSleep(const std::chrono::duration<Rep, Period>& rtime);


        /**
         * @brief
         * @tparam Ret
         * @param awaiter
         * @param context
         * @return
         */
        template< class Ret >
        Ret Await(std::shared_ptr<Result<Ret>> result);


        /**
         * @brief
         * @tparam Ret
         * @param results
         * @return
         */
//        template< class Ret >
//        std::vector<Ret> Await(std::vector<std::shared_ptr<Result<Ret>>> results);


        /**
         * @brief
         * @tparam Ret
         * @param awaiter
         * @param context
         * @return
         */
        template< class Ret >
        Ret Await(std::shared_ptr<Result<Ret>> result, CoroutineHandler* handler);


        [[nodiscard]] const Executor* GetMainExecutor() const { return main_executor; }
        Executor* GetMainExecutor() { return main_executor; }


        template< typename ExecutorType>
        const ExecutorType* GetExecutor() const {
            const std::type_info& eti = typeid(ExecutorType);
            return ((ExecutorType*)executors.at(eti.hash_code()));
        }


        template< typename ExecutorType>
        ExecutorType* GetExecutor() {
            const std::type_info& eti = typeid(ExecutorType);
            return ((ExecutorType*)executors.at(eti.hash_code()));
        }


        ObjectID GetWorkGroup(const std::string& name) const;

        EntityTag AddEntityTag(void *ptr);

        void DeleteEntityTag(EntityTag tag);

        std::shared_ptr< Mon::Counter > MakeMetricsCounter(const std::string & name, const std::map<std::string, std::string> &labels);
    protected:
        std::shared_ptr<Mon::Counter>   coroutine_counter;
    private:
        void SetupWorkGroups(const std::vector<WorkGroupOption>& work_groups_option);
        void CheckRuntime();


        /**
         * @brief
         * @param task
         */
        void Post(Task * task);


        /**
         * @brief
         */
        void CreateDefaultExecutors(int virtual_numa_nodes_count = 0);


        /**
         * @brief
         * @param tag
         * @return
         */
        IExecutor *FetchExecutor(const EntityTag & tag);
        IExecutor *FetchFreeExecutor(ExecutorType type);

        std::vector<WorkGroupOption>                work_groups_option;
        std::map<size_t , IExecutor*>               executors;
        std::map<EntityTag, IExecutor*>             entities_map;
        Executor*                                   main_executor;
        IOExecutor*                                 io_executor;
        bool                                        is_setup;
        std::unique_ptr<Mon::IMetricer>             metricer;
        std::mutex                                  entities_mutex;
    };


    template<class Callable, class... Arguments>
    auto Runtime::Async(Callable &&f, Arguments &&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>> {
        CheckRuntime();
        auto task = MakeTask(std::bind(std::forward<Callable>(f), std::forward<Arguments>(args)...));
        auto result = task->GetResult();
        Post(task);
        return result;
    }


    template<class Callable, class... Arguments>
    auto Runtime::AsyncDelayed(Callable&& f, Timespan delay_ms, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>> {
        CheckRuntime();
        auto task = MakeTask(std::bind(std::forward<Callable>(f), std::forward<Arguments>(args)...));
        auto result = task->GetResult();
        task->template SetDelay<Timestamp::Milli>(delay_ms);
        Post(task);
        return result;
    }


    template<class CoroutineType>
    std::shared_ptr<Result<typename CoroutineType::RetType>> Runtime::Async(CoroutineType & coroutine) {
        CheckRuntime();
        if(coroutine.GetState() != CoroutineState::kWaiting) {
            auto result = coroutine.GetResult();
            result->Wait();
        }

        Task* task = coroutine.MakeExecTask();
        if(task != nullptr) {
            coroutine.MakeResult();
            auto result = coroutine.GetResult();
            Post(task);
            return result;
        }else{
            return {};
        }
    }


    template< typename Rep, typename Period >
    inline ResultVoidPtr Runtime::AsyncSleep(const std::chrono::duration<Rep, Period>& rtime) {
        CheckRuntime();
        auto task = MakeDummyTask();
        task->template SetDelay< std::chrono::duration<Rep, Period> >(rtime.count());
        auto result = task->GetResult();
        Post(task);
        return result;
    }


    template< typename IOTaskType,
            class... Arguments >
    std::shared_ptr<Result<IOResult>> Runtime::AsyncIO(Arguments&&... args) {
        static_assert(std::is_base_of<IOTask, IOTaskType>::value, "ProcessorType must derive from IProcessor");
        CheckRuntime();
        IOTask *task = new IOTaskType(std::forward<Arguments>(args)...);
        auto result = task->GetResult();
        io_executor->Post(task);
        return result;
    }


    template<typename ExecutorType, typename TaskType, class... Arguments>
    std::shared_ptr<Result<typename TaskType::return_type>> Runtime::AsyncTask(Arguments &&... args) {
        static_assert(std::is_base_of<Task, TaskType>::value, "TaskType must derive from Task");
        CheckRuntime();
        auto *task = new TaskType(std::forward<Arguments>(args)...);
        auto result = task->GetResult();
        const std::type_info& eti = typeid(ExecutorType);
        ((ExecutorType*)executors.at(eti.hash_code()))->Post(task);
        return result;
    }

    template<typename ExecutorType, typename TaskType>
    void Runtime::AsyncPostTask(TaskType *task) {
        static_assert(std::is_base_of<Task, TaskType>::value, "TaskType must derive from Task");
        CheckRuntime();
        const std::type_info& eti = typeid(ExecutorType);
        ((ExecutorType*)executors.at(eti.hash_code()))->Post(task);
    }


    template<class Ret>
    Ret Runtime::Await(std::shared_ptr<Result<Ret>> result) {
        CheckRuntime();
        assert(result);

        result->Wait();
        return result->Get();
    }

    template<class Ret>
    Ret Runtime::Await(std::shared_ptr<Result<Ret>> result, CoroutineHandler* handler) {
        CheckRuntime();
        assert(result);
        assert(handler != nullptr);

        return Awaiter::Await<Ret>(result, [this](Result<Ret> *r, void *p) {
            if(p != nullptr) {
                auto resumed_coroutine = (CoroutineHandler*)p;
                auto task = resumed_coroutine->MakeExecTask();
                if(task != nullptr) {
                    Post(task);
                }
            }
        }, handler);
    }


    template < typename ExecutorType,
            class... Arguments >
    ExecutorType* Runtime::CreateExecutor(Arguments &&... args) {
        static_assert(std::is_base_of<IExecutor, ExecutorType>::value, "ExecutorType must derive from IExecutor");
        const std::type_info& eti = typeid(ExecutorType);
        auto *executor = new ExecutorType(std::forward<Arguments>(args)...);
        executors.insert(std::make_pair(eti.hash_code(), executor));
        return executor;
    }

    template<class CounterT>
    void Runtime::CreateMetricer(const std::map<std::string, std::string> &labels) {
        metricer = std::make_unique<Mon::Metricer<CounterT>>(labels);
    }

    /**
     * @brief
     */
    inline void SetupRuntime(const RuntimeOptions& options = {}) {
        return Runtime::g_runtime.Setup(options);
    }


    /**
     * @brief
     */
    inline void Terminate() {
        return Runtime::g_runtime.Terminate();
    }


    /**
     * @brief
     * @tparam ExecutorType
     * @tparam Arguments
     * @param args
     * @return
     */
    template < typename ExecutorType,
            class... Arguments >
    inline ExecutorType* CreateExecutor(Arguments&&... args) {
        return Runtime::g_runtime.CreateExecutor<ExecutorType>(std::forward<Arguments>(args)...);
    }

    /**
     * @brief
     */
    template< class CounterT >
    inline void CreateMetricer(const std::map<std::string, std::string> &labels) {
        Runtime::g_runtime.CreateMetricer<CounterT>(labels);
    }

    /**
     * @brief
     * @param name
     * @return
     */
    inline ObjectID GetWorkGroup(const std::string &name) {
        return Runtime::g_runtime.GetWorkGroup(name);
    }

    /**
     * @brief
     * @param ptr
     * @return
     */
    inline EntityTag AddEntityTag(void *ptr) {
        return Runtime::g_runtime.AddEntityTag(ptr);
    }

    /**
     * @brief
     * @param tag
     */
    inline void DeleteEntityTag(EntityTag tag) {
        return Runtime::g_runtime.DeleteEntityTag(tag);
    }

    /**
     * @brief
     * @tparam Callable
     * @tparam Arguments
     * @param f
     * @param args
     * @return
     */
    template <  class Callable,
            class... Arguments>
    inline auto Async(Callable&& f, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>> {
        return Runtime::g_runtime.Async(std::forward<Callable>(f), std::forward<Arguments>(args)...);
    }


    /**
     * @brief async call
     * @tparam CoroutineType
     */
    template<class CoroutineType >
    inline std::shared_ptr<Result<typename CoroutineType::RetType>> Async(CoroutineType & coroutine) {
        return Runtime::g_runtime.Async(coroutine);
    }

    /**
     * @brief
     * @tparam ExecutorType
     * @tparam TaskType
     * @param task
     */
    template<typename ExecutorType, typename TaskType>
    inline void AsyncPostTask(TaskType *task) {
        return Runtime::g_runtime.AsyncPostTask<ExecutorType, TaskType>(task);
    }


    /**
     * @brief async sleep
     * @tparam _Rep
     * @tparam _Period
     * @param rtime
     */
    template< typename Rep, typename Period >
    inline ResultVoidPtr AsyncSleep(const std::chrono::duration<Rep, Period>& rtime) {
        return Runtime::g_runtime.AsyncSleep<Rep, Period>(rtime);
    }


    /**
     * @brief async open file
     * @param stream
     * @param filename
     * @param flags
     * @param mods
     * @return
     */
    inline IOResultPtr AsyncFsOpen(const char* filename, int flags = O_RDWR | O_CREAT, int mode = S_IRWXU) {
        return Runtime::g_runtime.AsyncIO<FsOpenTask>(filename, flags, mode);
    }


    /**
     * @brief async close file
     * @param stream
     * @return
     */
    inline IOResultPtr AsyncFsClose(int fd) {
        return Runtime::g_runtime.AsyncIO<FsCloseTask>(fd);
    }


    /**
     * @brief async read from file
     * @param stream
     * @param offset
     * @return
     */
    inline IOResultPtr AsyncFsRead(int fd, const IOStreamPtr& stream, int64_t seek = -1, int64_t size = -1) {
        return Runtime::g_runtime.AsyncIO<FsReadTask>(fd, stream, seek);
    }


    /**
     * @brief async write to file
     * @param stream
     * @param offset
     * @return
     */
    inline IOResultPtr AsyncFsWrite(int fd, const IOStreamPtr& stream, int64_t seek = -1) {
        return Runtime::g_runtime.AsyncIO<FsWriteTask>(fd, stream, seek);
    }


    /**
     * @brief
     * @param handle_connection
     * @return
     */
    inline IOResultPtr AsyncConnect(const TCPConnectionPtr& connection) {
        return Runtime::g_runtime.AsyncIO<NetConnectionTask>(connection);
    }


    /**
     * @brief
     * @param server
     * @param callback
     * @return
     */
    inline IOResultPtr AsyncListen(const TCPServerPtr& server, const TCPSession::CallbackType& callback) {
        return Runtime::g_runtime.AsyncIO<NetListenTask>(server, callback);
    }


    /**
     * @brief
     * @param connection
     * @param stream
     * @return
     */
    inline IOResultPtr AsyncRead(const TCPConnectionPtr & connection, const IOStreamPtr & stream) {
        return Runtime::g_runtime.AsyncIO<NetReadTask>(connection, stream);
    }


    /**
     * @brief
     * @param connection
     * @param stream
     * @return
     */
    inline IOResultPtr AsyncWrite(const TCPConnectionPtr & connection, const IOStreamPtr & stream) {
        return Runtime::g_runtime.AsyncIO<NetWriteTask>(connection, stream);
    }


    /**
     * @brief
     * @param connection
     * @return
     */
    inline IOResultPtr AsyncClose(const TCPConnectionPtr & connection) {
        return Runtime::g_runtime.AsyncIO<NetCloseTask>(connection);
    }


    /**
     * @brief
     * @param info
     * @return
     */
    inline IOResultPtr AsyncNetAddrInfo(const NetAddrInfoPtr & info) {
        return Runtime::g_runtime.AsyncIO<NetAddrInfoTask>(info);
    }


    /**
     * @brief async bind udp socket
     * @param udp
     * @param broadcast
     * @return
     */
    inline IOResultPtr AsyncUDPBind(const UDPPtr & udp, int flags = 0, bool broadcast = false) {
        return Runtime::g_runtime.AsyncIO<NetUDPBindTask>(udp, flags, broadcast);
    }


    /**
     * @brief async send to address
     * @param udp
     * @param stream
     * @param send_addr
     * @return
     */
    inline IOResultPtr AsyncSend(const UDPPtr & udp, const IOStreamPtr & stream, const IPv4Addr &send_addr) {
        return Runtime::g_runtime.AsyncIO<NetSendTask>(udp, stream, send_addr);
    }


    /**
     * @brief
     * @param udp
     * @param stream
     * @return
     */
    inline IOResultPtr AsyncRecv(const UDPPtr & udp, const IOStreamPtr & stream) {
        return Runtime::g_runtime.AsyncIO<NetRecvTask>(udp, stream);
    }


    /**
     * @brief await
     * @tparam Ret
     * @param awaiter
     * @param context
     * @return
     */
    template< class Ret >
    inline Ret Await(std::shared_ptr<Result<Ret>> result) {
        return Runtime::g_runtime.Await(result);
    }

    /**
     * @brief await
     * @tparam Ret
     * @param awaiter
     * @param context
     * @return
     */
    template< class Ret >
    inline Ret Await(std::shared_ptr<Result<Ret>> result, CoroutineHandler* handler) {
        return Runtime::g_runtime.Await<Ret>(result, handler);
    }
}


#endif //AR_RUNTIME_H
