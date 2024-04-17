#ifndef AR_DATAFLOW_NOTIFYER_H
#define AR_DATAFLOW_NOTIFYER_H

#include "ar/ar.hpp"
#include "ar/notifier.hpp"
#include <typeinfo>

namespace AsyncRuntime::Dataflow {

    /**
     * @class Notifyer
     * @brief
     */
    class Notifier {
        typedef std::shared_ptr<AsyncRuntime::Result<int>> ResultPtr;
    public:
        Notifier() = default;

        void Notify(int state);

        template< typename... Arguments >
        ResultPtr AsyncWatch(Arguments &&... args);
        ResultPtr AsyncWatchAll();

        template <typename T>
        static bool HasState(int states, T state) { return ((states & (int)state) == (int)state) == 1; }
    private:
        void CheckNotifications(int & notifications, int state) const;
        ResultPtr MakeWatcher();
        ResultPtr MakeResult(int state);
        ResultPtr Watch();

        std::mutex mutex;
        ResultPtr async_watcher;
        bool watch_all = false;
        uint8_t watch_state = {0};
        uint8_t notify_state = {0};
    };

    template<typename... Arguments>
    Notifier::ResultPtr Notifier::AsyncWatch(Arguments &&... args) {
        std::lock_guard<std::mutex> lock(mutex);
        watch_all = false;
        int notifications = 0;
        ((void) CheckNotifications(notifications, (int)std::forward<Arguments>(args)), ...);
        if (notifications == 0) {
            watch_state = 0;
            notify_state = 0;
            ((watch_state |= (int)(std::forward<Arguments>(args))), ...);
            return Watch();
        } else {
            watch_state = 0;
            notify_state = 0;
            ((watch_state |= (int)(std::forward<Arguments>(args))), ...);
            return MakeResult(notifications);
        }
    }
}

#endif //AR_DATAFLOW_NOTIFYER_H
