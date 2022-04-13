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
                                                  IOFsStreamPtr stream,
                                                  CoroutineHandler* handler = nullptr);


        /**
         * @brief
         * @return
         */
        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncNet(Method method,
                                                   TCPServerPtr server,
                                                   CoroutineHandler* handler = nullptr);

        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncNet(Method method,
                                                   TCPSessionPtr session,
                                                   IOFsStreamPtr stream,
                                                   CoroutineHandler* handler = nullptr);

        template<typename Method>
        std::shared_ptr<Result<IOResult>> AsyncNet(Method method,
                                                   TCPSessionPtr session,
                                                   CoroutineHandler* handler = nullptr);

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
        void ApplyAsyncIOHandler(IOTask *task, CoroutineHandler* handler);
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
        Executor*                                   main_executor;
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
                                                       IOFsStreamPtr stream,
                                                       CoroutineHandler* handler) {
        CheckRuntime();
        auto *task = new IOFsTaskImpl<Method>(method, stream);
        auto result = task->GetResult();
        ApplyAsyncIOHandler(task, handler);
        io_executor->Post(task);
        return result;
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncNet(Method method,
                                                        TCPServerPtr server,
                                                        CoroutineHandler* handler) {
        CheckRuntime();
        auto *task = new IONetTaskImpl<Method>(method, server);
        auto result = task->GetResult();
        ApplyAsyncIOHandler(task, handler);
        io_executor->Post(task);
        return result;
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncNet(Method method,
                                                        TCPSessionPtr session,
                                                        IOFsStreamPtr stream,
                                                        CoroutineHandler* handler) {
        CheckRuntime();
        auto *task = new IONetTaskImpl<Method>(method, session, stream);
        auto result = task->GetResult();
        ApplyAsyncIOHandler(task, handler);
        io_executor->Post(task);
        return result;
    }


    template<typename Method>
    std::shared_ptr<Result<IOResult>> Runtime::AsyncNet(Method method,
                                                        TCPSessionPtr session,
                                                        CoroutineHandler* handler) {
        CheckRuntime();
        auto *task = new IONetTaskImpl<Method>(method, session);
        auto result = task->GetResult();
        ApplyAsyncIOHandler(task, handler);
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
    inline IOResultPtr AsyncFsOpen(const IOFsStreamPtr& stream, const char* filename, int flags = O_RDWR | O_CREAT, int mode = S_IRWXU) {
        return Runtime::g_runtime.AsyncFs<IOFsOpen>(IOFsOpen{filename, flags, mode}, stream);
    }


    /**
     * @brief async close file
     * @param stream
     * @return
     */
    inline IOResultPtr AsyncFsClose(const IOFsStreamPtr& stream) {
        return Runtime::g_runtime.AsyncFs<IOFsClose>(IOFsClose{}, stream);
    }


    /**
     * @brief async read from file
     * @param stream
     * @param offset
     * @return
     */
    inline IOResultPtr AsyncFsRead(const IOFsStreamPtr& stream, int64_t seek = -1, int64_t size = -1) {
        return Runtime::g_runtime.AsyncFs<IOFsRead>(IOFsRead{seek, size}, stream);
    }


    /**
     * @brief async write to file
     * @param stream
     * @param offset
     * @return
     */
    inline IOResultPtr AsyncFsWrite(const IOFsStreamPtr& stream, int64_t seek = -1) {
        return Runtime::g_runtime.AsyncFs<IOFsWrite>(IOFsWrite{seek}, stream);
    }


    /**
     * @brief
     * @param server
     * @return
     */
    inline IOResultPtr AsyncListen(const TCPServerPtr& server, const std::function<void(TCPSessionPtr)>& handle_connection) {
        return Runtime::g_runtime.AsyncNet<TCPListen>(TCPListen{ 0, handle_connection }, server);
    }


    /**
     * @brief
     * @param server
     * @return
     */
    inline IOResultPtr AsyncRead(const TCPSessionPtr & session, const IOFsStreamPtr & stream) {
        return Runtime::g_runtime.AsyncNet<NetRead>(NetRead{ }, session, stream);
    }


    /**
     * @brief
     * @param session
     * @return
     */
    inline IOResultPtr AsyncClose(const TCPSessionPtr & session) {
        return Runtime::g_runtime.AsyncNet<NetClose>(NetClose{ }, session);
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


    template< class Ret >
    inline Ret Await(std::shared_ptr<Result<Ret>> result, const TCPSessionPtr & session) {
        return Runtime::g_runtime.Await<Ret, Result<Ret>>(result, session->GetCoroutineHandler());
    }


    template< class Ret >
    inline Ret Await(std::shared_ptr<ChannelReceiver<Ret>> result, CoroutineHandler* handler) {
        return Runtime::g_runtime.Await<Ret, ChannelReceiver<Ret>>(result, handler);
    }


    template< class Ret >
    inline Ret Await(std::shared_ptr<ChannelReceiver<Ret>> result, const TCPSessionPtr & session) {
        return Runtime::g_runtime.Await<Ret, ChannelReceiver<Ret>>(result, session->GetCoroutineHandler());
    }
}


#endif //AR_RUNTIME_H
