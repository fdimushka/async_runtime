#include "ar/task.hpp"

using namespace AsyncRuntime;

future_t<void> make_resolved_future() {
    promise_t<void> p;
    p.set_value();
    return p.get_future();
}