#ifndef AR_DATAFLOW_NOTIFYER_H
#define AR_DATAFLOW_NOTIFYER_H

#include "ar/ar.hpp"
#include "ar/notifier.hpp"
#include <typeinfo>

namespace AsyncRuntime::Dataflow {

    class Notifier {
    public:
        Notifier() = default;

        void Notify(int state);

        template< typename... Arguments >
        future_t<int> AsyncWatch(Arguments &&... args);
        future_t<int> AsyncWatchAny();

        template <typename T>
        static bool HasState(int states, T state) { return ((states & (int)state) == (int)state) == 1; }
    private:
        void CheckNotifications(int & notifications, int state) const;

        std::mutex mutex;
        promise_t<int> promise;
        bool notified = false;
        bool watch_any = false;
        uint8_t watch_state = {0};
        uint8_t notify_state = {0};
    };

    template<typename... Arguments>
    future_t<int> Notifier::AsyncWatch(Arguments &&... args) {
        std::lock_guard<std::mutex> lock(mutex);
        watch_any = false;
        int notifications = 0;
        ((void) CheckNotifications(notifications, (int)std::forward<Arguments>(args)), ...);
        if (notifications == 0) {
            watch_state = 0;
            notify_state = 0;
            ((watch_state |= (int)(std::forward<Arguments>(args))), ...);
            promise = {};
            notified = false;
            return promise.get_future();
        } else {
            watch_state = 0;
            notify_state = 0;
            ((watch_state |= (int)(std::forward<Arguments>(args))), ...);
            notified = false;
            promise = {};
            return make_resolved_future(notifications);
        }
    }
}

#endif //AR_DATAFLOW_NOTIFYER_H
