#ifndef AR_CHANNEL_H
#define AR_CHANNEL_H


#include "ar/task.hpp"
#include "ar/object.hpp"
#include "ar/resource_pool.hpp"
#include "ar/allocators.hpp"

#include <cstring>
#include <queue>

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
        ~Watcher() override = default;

        std::optional<T> TryReceive();
        future_t<void>   AsyncWait();
    private:
        void Send(const T& msg);

        std::queue<T>                                   queue;
        std::mutex                                      mutex;
        promise_t<void>                                 promise;
        bool                                            resolved = false;
    };


    template<typename T>
    class Channel {
        static_assert(std::is_copy_constructible_v<T>);
#ifdef USE_TESTS
        friend CHANNEL_TEST_FRIEND;
#endif
    public:
        Channel() = default;
        explicit Channel(resource_pool *resource);

        typedef Watcher<T>  WatcherType;

        void Send(const T& msg);

        std::shared_ptr<WatcherType> Watch();

        std::shared_ptr<WatcherType> Watch(resource_pool *resource);

        void UnWatch(const std::shared_ptr<WatcherType>& watcher);
    private:
        std::mutex                                             mutex;
        map<ObjectID, std::shared_ptr<WatcherType>>            watchers;
    };

    template<typename T>
    Channel<T>::Channel(resource_pool *resource) : watchers(resource) { }

    template<typename T>
    std::shared_ptr<Watcher<T>> Channel<T>::Watch() {
        std::lock_guard<std::mutex> lock(mutex);
        auto watcher = std::make_shared<Watcher<T>>();
        watchers.insert(std::make_pair(watcher->GetID(), watcher));
        return watcher;
    }

    template<typename T>
    std::shared_ptr<Watcher<T>> Channel<T>::Watch(resource_pool *resource) {
        std::lock_guard<std::mutex> lock(mutex);
        auto watcher = make_shared_ptr<Watcher<T>>(resource);
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
                T new_msg(msg);
                watcher->Send(new_msg);
                ++it;
            } else {
                it = watchers.erase(it);
            }
        }
    }


    template<typename T>
    void Watcher<T>::Send(const T& msg) {
        std::lock_guard<std::mutex>  lock(mutex);
        queue.push(msg);
        if (!resolved) {
            promise.set_value();
            resolved = true;
        }
    }


    template<typename T>
    std::optional<T> Watcher<T>::TryReceive() {
        std::lock_guard<std::mutex>  lock(mutex);
        if (!queue.empty()) {
            T v = queue.front();
            queue.pop();
            return v;
        } else {
            return std::nullopt;
        }
    }


    template<typename T>
    future_t<void> Watcher<T>::AsyncWait() {
        std::lock_guard<std::mutex>  lock(mutex);
        if (queue.empty()) {
            resolved = false;
            promise = {};
            return promise.get_future();
        } else {
            resolved = false;
            promise = {};
            return make_resolved_future();
        }
    }
}

#endif //AR_CHANNEL_H
