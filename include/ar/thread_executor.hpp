#ifndef AR_THREADEXECUTOR_H
#define AR_THREADEXECUTOR_H

#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>
#include <memory>

namespace AsyncRuntime {


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
            threads.emplace_back(std::move(std::thread(std::forward<Fn>(fn), std::forward<Args>(args)...)));
        }


        /**
         * @brief
         */
        void Join();


        /**
         * @brief
         * @return
         */
        [[nodiscard]] const std::vector<std::thread>& GetThreads() const { return threads; }


        /**
         * @brief
         * @return
         */
        [[nodiscard]] std::vector<std::thread::id> GetThreadIds() const;
    private:
        std::vector<std::thread>     threads;
    };


    typedef std::shared_ptr<ThreadExecutor> ThreadExecutorPtr;
}

#endif //AR_THREADEXECUTOR_H
