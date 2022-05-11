#ifndef AR_RUNTIME_H
#define AR_RUNTIME_H

#include "ar/work_steal_queue.h"
#include "ar/task.hpp"
#include "ar/notifier.hpp"
#include "ar/coroutine.hpp"
#include "ar/awaiter.hpp"
#include "ar/channel.hpp"
#include "ar/executor.hpp"
#include "ar/io_task.hpp"
#include "ar/io_executor.hpp"


namespace AsyncRuntime {
    /**
     * @class Runtime
     * @brief runtime class
     */
    class Runtime {
        friend class Processor;
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
         */
        void Setup(/*...*/);
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
                    class... Arguments  >
        auto Async(Callable&& f, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>>;


        /**
         * @brief async call
         * @tparam Callable
         * @tparam Arguments
         * @param wg
         * @param f
         * @param args
         * @return
         */
        template <  class Callable,
                    class... Arguments  >
        auto Async(IExecutor* ex, Callable&& f, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>>;


        /**
         * @brief async call
         * @tparam CoroutineType
         */
        template<class CoroutineType>
        std::shared_ptr<Result<typename CoroutineType::RetType>> Async(CoroutineType & coroutine);


        /**
         * @brief
         * @tparam CoroutineType
         * @param coroutine
         * @return
         */
        template<class CoroutineType>
        std::shared_ptr<Result<typename CoroutineType::RetType>> Async(IExecutor* ex, CoroutineType & coroutine);


        /**
         * @brief async sleep
         * @tparam _Rep
         * @tparam _Period
         * @param rtime
         */
        template<typename Rep, typename Period>
        ResultVoidPtr AsyncSleep(const std::chrono::duration<Rep, Period>& rtime);


        /**
         * @brief
         * @return
         */
        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncFs(Method method,
                                                  const IOStreamPtr& stream);


        /**
         * @brief
         * @return
         */
        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncNet(Method method,
                                                   const TCPServerPtr& server);


        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncNet(Method method,
                                                   const TCPConnectionPtr& connection);


        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncNet(Method method,
                                                   const IOStreamPtr& stream);

        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncNet(Method method,
                                                   const NetAddrInfoPtr& info);

        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncNet(Method method);


        /**
         * @brief
         * @tparam Ret
         * @param awaiter
         * @param context
         * @return
         */
        template<class Ret>
        Ret Await(std::shared_ptr<Result<Ret>> result);


        /**
         * @brief
         * @tparam Ret
         * @param awaiter
         * @param context
         * @return
         */
        template<class Ret, class Res>
        Ret Await(std::shared_ptr<Res> result, CoroutineHandler* handler);


        /**
         * @brief
         */
        void Bind(const TCPServerPtr & server, int flags = 0);
    private:
        void CheckRuntime();

        /**
         * @brief
         * @param task
         */
        void Post(Task * task);


        /**
         * @brief
         * @param task
         */
        void Post(IExecutor* ex, Task * task);


        /**
         * @brief
         */
        void CreateDefaultExecutors();


        std::unordered_map<ObjectID, IExecutor*>    executors;
        IExecutor*                                  main_executor;
        IOExecutor*                                 io_executor;
        bool                                        is_setup;
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
    auto Runtime::Async(IExecutor* ex, Callable &&f, Arguments &&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>> {
        CheckRuntime();
        auto task = MakeTask(std::bind(std::forward<Callable>(f), std::forward<Arguments>(args)...));
        auto result = task->GetResult();
        Post(ex, task);
        return result;
    }


    template<class CoroutineType>
    std::shared_ptr<Result<typename CoroutineType::RetType>> Runtime::Async(CoroutineType & coroutine) {
        CheckRuntime();
        if(coroutine.GetState() != CoroutineState::kWaiting) {
            auto result = coroutine.GetResult();
            result->Wait();
        }

        auto task = coroutine.MakeExecTask();
        if(task != nullptr) {
            coroutine.MakeResult();
            auto result = coroutine.GetResult();
            Post(task);
            return result;
        }else{
            return {};
        }
    }


    template<class CoroutineType>
    std::shared_ptr<Result<typename CoroutineType::RetType>> Runtime::Async(IExecutor* ex, CoroutineType & coroutine) {
        CheckRuntime();
        if(coroutine.GetState() != CoroutineState::kWaiting) {
            auto result = coroutine.GetResult();
            result->Wait();
        }

        auto task = coroutine.MakeExecTask();
        if(task != nullptr) {
            coroutine.MakeResult();
            auto result = coroutine.GetResult();
            Post(ex, task);
            return result;
        }else{
            return {};
        }
    }


    template<typename Rep, typename Period>
    ResultVoidPtr Runtime::AsyncSleep(const std::chrono::duration<Rep, Period>& rtime) {
        CheckRuntime();
        return std::move(Async([](const std::chrono::duration<Rep, Period>& t){
            std::this_thread::sleep_for(t);
        }, rtime));
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncFs(Method method,
                                                       const IOStreamPtr& stream) {
        CheckRuntime();
        auto *task = new IOFsTaskImpl<Method>(method, stream);
        auto result = task->GetResult();
        io_executor->Post(task);
        return result;
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncNet(Method method,
                                                        const TCPServerPtr& server) {
        CheckRuntime();
        auto *task = new IONetTaskImpl<Method>(method, server);
        auto result = task->GetResult();
        io_executor->Post(task);
        return result;
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncNet(Method method,
                                                        const IOStreamPtr& stream) {
        CheckRuntime();
        auto *task = new IONetTaskImpl<Method>(method,  stream);
        auto result = task->GetResult();
        io_executor->Post(task);
        return result;
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncNet(Method method,
                                                        const TCPConnectionPtr& connection) {
        CheckRuntime();
        auto *task = new IONetTaskImpl<Method>(method, connection);
        auto result = task->GetResult();
        io_executor->Post(task);
        return result;
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncNet(Method method,
                                                        const NetAddrInfoPtr& info) {
        CheckRuntime();
        auto *task = new IONetTaskImpl<Method>(method, info);
        auto result = task->GetResult();
        io_executor->Post(task);
        return result;
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncNet(Method method) {
        CheckRuntime();
        auto *task = new IONetTaskImpl<Method>(method);
        auto result = task->GetResult();
        io_executor->Post(task);
        return result;
    }


    template<class Ret>
    Ret Runtime::Await(std::shared_ptr<Result<Ret>> result) {
        CheckRuntime();
        assert(result);

        result->Wait();
        return result->Get();
    }


    template<class Ret, class Res>
    Ret Runtime::Await(std::shared_ptr<Res> result, CoroutineHandler* handler) {
        CheckRuntime();
        assert(result);
        assert(handler != nullptr);

        return Awaiter::Await(result, [this](void* p) {
            if(p != nullptr) {
                auto resumed_coroutine = (CoroutineHandler*)p;
                auto task = resumed_coroutine->MakeExecTask();
                if(task != nullptr) {
                    Post(task);
                }
            }
        }, handler);
    }




    /**
     * @brief
     */
    inline void SetupRuntime(/*args...*/) {
        return Runtime::g_runtime.Setup();
    }


    /**
     * @brief
     */
    inline void Terminate() {
        return Runtime::g_runtime.Terminate();
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
                class... Arguments  >
    inline auto Async(Callable&& f, Arguments&&... args) -> std::shared_ptr<Result<decltype(std::forward<Callable>(f)(std::forward<Arguments>(args)...))>> {
        return Runtime::g_runtime.Async(std::forward<Callable>(f), std::forward<Arguments>(args)...);
    }


    /**
     * @brief async call
     * @tparam CoroutineType
     */
    template< class CoroutineType >
    inline std::shared_ptr<Result<typename CoroutineType::RetType>> Async(CoroutineType & coroutine) {
        return Runtime::g_runtime.Async(coroutine);
    }


    /**
     * @brief async sleep
     * @tparam _Rep
     * @tparam _Period
     * @param rtime
     */
    template< typename Rep, typename Period >
    inline ResultVoidPtr AsyncSleep(const std::chrono::duration<Rep, Period>& rtime) {
        return Runtime::g_runtime.AsyncSleep(rtime);
    }


    /**
     * @brief async open file
     * @param stream
     * @param filename
     * @param flags
     * @param mods
     * @return
     */
    inline IOResultPtr AsyncFsOpen(const IOStreamPtr& stream, const char* filename, int flags = O_RDWR | O_CREAT, int mode = S_IRWXU) {
        return Runtime::g_runtime.AsyncFs<IOFsOpen>(IOFsOpen{filename, flags, mode}, stream);
    }


    /**
     * @brief async close file
     * @param stream
     * @return
     */
    inline IOResultPtr AsyncFsClose(const IOStreamPtr& stream) {
        return Runtime::g_runtime.AsyncFs<IOFsClose>(IOFsClose{}, stream);
    }


    /**
     * @brief async read from file
     * @param stream
     * @param offset
     * @return
     */
    inline IOResultPtr AsyncFsRead(const IOStreamPtr& stream, int64_t seek = -1, int64_t size = -1) {
        return Runtime::g_runtime.AsyncFs<IOFsRead>(IOFsRead{seek, size}, stream);
    }


    /**
     * @brief async write to file
     * @param stream
     * @param offset
     * @return
     */
    inline IOResultPtr AsyncFsWrite(const IOStreamPtr& stream, int64_t seek = -1) {
        return Runtime::g_runtime.AsyncFs<IOFsWrite>(IOFsWrite{seek}, stream);
    }


    /**
     * @brief
     * @param handle_connection
     * @return
     */
    inline IOResultPtr AsyncConnect(const TCPConnectionPtr& connection) {
        return Runtime::g_runtime.AsyncNet<IONetConnect>(IONetConnect{  }, connection);
    }


    /**
     * @brief
     * @param server
     * @return
     */
    inline IOResultPtr AsyncListen(const TCPServerPtr& server, const TCPSession::HandlerType & handle_connection) {
        return Runtime::g_runtime.AsyncNet<IONetListen>(IONetListen{ 0, handle_connection }, server);
    }


    /**
     * @brief
     * @param server
     * @return
     */
    inline IOResultPtr AsyncRead(const TCPSessionPtr & session, const IOStreamPtr & stream) {
        return Runtime::g_runtime.AsyncNet<IONetRead>(IONetRead{ session->GetClient() }, stream);
    }


    inline IOResultPtr AsyncRead(const TCPConnectionPtr & connection, const IOStreamPtr & stream) {
        return Runtime::g_runtime.AsyncNet<IONetRead>(IONetRead{ &connection->socket }, stream);
    }


    /**
     * @brief
     * @param session
     * @param stream
     * @return
     */
    inline IOResultPtr AsyncWrite(const TCPSessionPtr & session, const IOStreamPtr & stream) {
        return Runtime::g_runtime.AsyncNet<IONetWrite>(IONetWrite{ session->GetClient() }, stream);
    }


    inline IOResultPtr AsyncWrite(const TCPConnectionPtr & connection, const IOStreamPtr & stream) {
        return Runtime::g_runtime.AsyncNet<IONetWrite>(IONetWrite{ &connection->socket }, stream);
    }


    /**
     * @brief
     * @param session
     * @return
     */
    inline IOResultPtr AsyncClose(const TCPSessionPtr & session) {
        return Runtime::g_runtime.AsyncNet<IONetClose>(IONetClose{ session->GetClient()});
    }


    inline IOResultPtr AsyncClose(const TCPConnectionPtr & connection) {
        return Runtime::g_runtime.AsyncNet<IONetClose>(IONetClose{ &connection->socket });
    }


    /**
     * @brief
     * @param info
     * @return
     */
    inline IOResultPtr AsyncNetAddrInfo(const NetAddrInfoPtr & info) {
        return Runtime::g_runtime.AsyncNet<IONetAddrInfo>(IONetAddrInfo{  }, info);
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
        return Runtime::g_runtime.Await<Ret, Result<Ret>>(result, handler);
    }


    /**
     * @brief
     * @tparam Ret
     * @param result
     * @param handler
     * @return
     */
    template< class Ret >
    inline Ret Await(std::shared_ptr<ChannelReceiver<Ret>> result, CoroutineHandler* handler) {
        return Runtime::g_runtime.Await<Ret, ChannelReceiver<Ret>>(result, handler);
    }
}


#endif //AR_RUNTIME_H
