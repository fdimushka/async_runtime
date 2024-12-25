#ifndef AR_THREADEXECUTOR_H
#define AR_THREADEXECUTOR_H

#include "ar/cpu_helper.hpp"

#include <thread>
#include <memory>

namespace AsyncRuntime {


    class ThreadHelper {
    public:
        static void SetName(const char* name);
        static std::string GetName();
    };


    /**
     * @brief
     */
    class ThreadExecutor {
    public:
        ThreadExecutor() = default;
        virtual ~ThreadExecutor();


        ThreadExecutor(const ThreadExecutor&) = delete;
        ThreadExecutor& operator =(const ThreadExecutor&) = delete;
        ThreadExecutor(ThreadExecutor&&) = delete;
        ThreadExecutor& operator =(ThreadExecutor&&) = delete;


        /**
         * @brief
         * @tparam Fn
         * @tparam Args
         * @param fn
         * @param args
         */
        template<class Fn, class ... Args>
        void Submit(Fn&& fn, Args&& ... args)
        {
            thread = std::move(std::thread(std::forward<Fn>(fn), std::forward<Args>(args)...));
        }


        /**
         * @brief
         * @param affinity_cpu
         */
        int SetAffinity(const CPU & affinity_cpu);


        /**
         * @brief
         */
        void Join();


        /**
         * @brief
         * @return
         */
        [[nodiscard]] const std::thread& GetThread() const { return thread; }


        /**
         * @brief
         * @return
         */
        [[nodiscard]] std::thread::id GetThreadId() const { return thread.get_id(); }
    private:
        std::thread     thread;
    };


    typedef std::shared_ptr<ThreadExecutor> ThreadExecutorPtr;
}

#endif //AR_THREADEXECUTOR_H
