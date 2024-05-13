#include "ar/dataflow/notifier.hpp"
#include "ar/dataflow/kernel_events.hpp"

using namespace AsyncRuntime::Dataflow;

void Notifier::Notify(int state) {
    std::lock_guard<std::mutex> lock(mutex);
    notify_state |= state;
    if (watch_any || ((watch_state & state) == state) == 1) {
        if (!notified) {
            notified = true;
            promise.set_value(notify_state);
            return;
        }
    }
}

void Notifier::CheckNotifications(int &notifications, int state) const {
    if (((notify_state & state) == state) == 1) {
        notifications |= state;
    }
}

AsyncRuntime::future_t<int> Notifier::AsyncWatchAny() {
    std::lock_guard<std::mutex> lock(mutex);
    watch_any = true;
    if (notify_state == 0) {
        watch_state = 0;
        notify_state = 0;
        notified = false;
        promise = {};
        return promise.get_future();
    } else {
        int notifications = notify_state;
        watch_state = 0;
        notify_state = 0;
        notified = false;
        promise = {};
        return AsyncRuntime::make_resolved_future(notifications);
    }
}


