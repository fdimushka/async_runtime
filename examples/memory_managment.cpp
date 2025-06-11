#include <iostream>
#include <chrono>
#include <memory>
#include "ar/ar.hpp"
#include "ar/logger.hpp"
#include "ar/dataflow/dataflow.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include "ar/allocators.hpp"
#include "ar/flat_buffer.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;


template<typename T>
class BasePacket {
public:
    BasePacket(resource_pool *resource, int size) : buffer(Allocator<T>(resource), size) {
    }

    ~BasePacket() {
    }

private:
    flat_buffer<T, Allocator<T>> buffer;
};


typedef BasePacket<char> Packet;

void async_fun_a(coroutine_handler* handler, yield<void> & yield, Channel<std::shared_ptr<Packet>> *buffer) {
    yield();

    for(int i = 0; i < 1000; ++i) {
        auto packet = make_shared_ptr<Packet>(handler->get_resource(), handler->get_resource(), 100);
        buffer->Send(packet);
    }
    buffer->Send({});
}

void async_fun_b(coroutine_handler* handler, yield<void> & yield, Channel<std::shared_ptr<Packet>> *channel) {
    int i = 0;

    auto watcher = channel->Watch();

    for(;;) {
        auto v = watcher->TryReceive();
        if (v.has_value()) {
            std::shared_ptr<Packet> packet = v.value();
            if (packet ) {
                i++;
            } else {
                break;
            }
        } else {
            Await(watcher->AsyncWait(), handler);
        }
    }

    std::cout << i << std::endl;
}

int main() {
    AsyncRuntime::Logger::s_logger.SetStd();
    SetupRuntime();
    auto resource = AsyncRuntime::CreateResource();
    {

        Channel<std::shared_ptr<Packet>> ch;
        auto coro_a = make_coroutine(resource.get(), &async_fun_a, &ch);
        Await(Async(coro_a));
        auto coro_b = make_coroutine(resource.get(), &async_fun_b, &ch);
        auto result_async_fun = Async(coro_b);
        Async(coro_a);
        result_async_fun.wait();
    }
    AsyncRuntime::DeleteResource(resource.release());

    Terminate();
    return 0;
}

