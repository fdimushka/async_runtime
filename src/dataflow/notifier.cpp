#include "ar/dataflow/notifier.hpp"
#include "ar/dataflow/kernel_events.hpp"

using namespace AsyncRuntime::Dataflow;

Notifier::ResultPtr Notifier::MakeWatcher() {
    return std::make_shared<AsyncRuntime::Result<int>>();
}

Notifier::ResultPtr Notifier::MakeResult(int state) {
    return std::make_shared<AsyncRuntime::Result<int>>(state);
}

void Notifier::Notify(int state) {
    std::lock_guard<std::mutex> lock(mutex);
    notify_state |= state;
    if (watch_all || ((watch_state & state) == state) == 1) {
        if (async_watcher && !async_watcher->Resolved()) {
            int notifications = notify_state;
            notify_state = 0;
            async_watcher->SetValue(notifications);
        }
    }
}

void Notifier::CheckNotifications(int &notifications, int state) const {
    if (((notify_state & state) == state) == 1) {
        notifications |= state;
    }
}

Notifier::ResultPtr Notifier::Watch() {
    async_watcher = MakeWatcher();
    return async_watcher;
}

Notifier::ResultPtr Notifier::AsyncWatchAll() {
    std::lock_guard<std::mutex> lock(mutex);
    watch_all = true;
    if (notify_state == 0) {
        watch_state = 0;
        notify_state = 0;
        return Watch();
    } else {
        int notifications = notify_state;
        watch_state = 0;
        notify_state = 0;
        return MakeResult(notifications);
    }
}


