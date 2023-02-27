#ifndef AR_CHANNEL_H
#define AR_CHANNEL_H


#include "ar/task.hpp"
#include "ar/object.hpp"
#include "ar/array.hpp"
#include "ar/work_steal_queue.hpp"

#include <iostream>
#include <map>
#include <cstdint>
#include <cstring>
#include <atomic>

#ifdef USE_TESTS
class CHANNEL_TEST_FRIEND;
#endif


namespace AsyncRuntime {

    template<typename T>
    class Channel;

    /**
     * @brief
     * @class Channel
     */
    template<typename T>
    class Watcher : public BaseObject {
        friend Channel<T>;
    public:
        Watcher() = default;
        ~Watcher() override;

        std::optional<T*> Receive();
        std::optional<T*> TryReceive();
        std::shared_ptr<Result<T*>>   AsyncReceive();
    private:
        void Send(T *msg);

        WorkStealQueue<T*>                              queue;
        std::condition_variable                         cv;
        std::mutex                                      mutex;
        std::shared_ptr<Result<T*>>                     async_result_;
    };


    template<typename T>
    class Channel {
        static_assert(std::is_copy_constructible_v<T>);
#ifdef USE_TESTS
        friend CHANNEL_TEST_FRIEND;
#endif
    public:
        typedef Watcher<T>  WatcherType;

        /**
         * @brief
         * @param buff
         * @param size
         */
        void Send(const T& msg);


        /**
         * @brief
         * @param watcher_id
         */
        std::shared_ptr<WatcherType> Watch();


        /**
         * @brief
         * @param watcher
         */
        void UnWatch(const std::shared_ptr<WatcherType>& watcher);
    private:
        std::mutex                                             mutex;
        std::map<ObjectID, std::shared_ptr<WatcherType>>       watchers;
    };


    template<typename T>
    std::shared_ptr<Watcher<T>> Channel<T>::Watch() {
        std::lock_guard<std::mutex> lock(mutex);
        auto watcher = std::make_shared<Watcher<T>>();
        watchers.insert(std::make_pair(watcher->GetID(), watcher));
        return watcher;
    }


    template<typename T>
    void Channel<T>::UnWatch(const std::shared_ptr<WatcherType> &watcher) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = watchers.find(watcher->GetID());
        if(it != watchers.end())
            watchers.erase(it);
    }


    template<typename T>
    void Channel<T>::Send(const T& msg) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto it = watchers.begin(); it != watchers.end(); ) {
            const std::shared_ptr<WatcherType>& watcher = it->second;

            if (watcher.use_count() > 1) {
                auto* clone_msg = new T(msg);
                watcher->Send(clone_msg);
                ++it;
            } else {
                it = watchers.erase(it);
            }
        }
    }


    template<typename T>
    Watcher<T>::~Watcher() {
        while (!queue.empty()) {
            auto v = queue.steal();
            if(v) {
                delete v.value();
            }
        }

        //@TODO check and release async_result_
    }


    template<typename T>
    void Watcher<T>::Send(T *msg) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if( async_result_ && !async_result_->Resolved() ) {
                async_result_->SetValue(msg);
            }else{
                queue.push(msg);
            }
        }

        cv.notify_one();
    }


    template<typename T>
    std::optional<T*> Watcher<T>::Receive() {
        std::unique_lock<std::mutex>  lock(mutex);
        while(queue.empty()) {
            cv.wait(lock);
        }
        return queue.steal();
    }


    template<typename T>
    std::optional<T*> Watcher<T>::TryReceive() {
        return queue.steal();
    }


    template<typename T>
    std::shared_ptr<Result<T*>> Watcher<T>::AsyncReceive() {
        auto v = queue.steal();
        if(!v) {
            std::lock_guard<std::mutex> lock(mutex);
            //@TODO check and release prev async_result_
            async_result_ = std::make_shared<Result<T*>>();
            return async_result_;
        } else {
            return std::make_shared<Result<T*>>(v.value());
        }
    }
}

#endif //AR_CHANNEL_H
