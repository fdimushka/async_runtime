#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;

struct Packet {
    size_t size = 0;
};

static Packet long_computation(size_t input) {
    std::this_thread::sleep_for(500ms);
    return Packet{input};
}

static void on_long_computation_end(Result<Packet> *result, void *analytics_ptr);

class Analytics {
    class LongComputationControlBlock : public Result<Packet>::CallbackControlBlock {
    public:
        explicit LongComputationControlBlock(Analytics *ptr) : Result<Packet>::CallbackControlBlock(on_long_computation_end, ptr) { }

        void PushComputationResult(Packet packet) {
            std::lock_guard<std::mutex> lock(mutex);
            results.push_back(packet);
            computed_packets_count++;
        }

        void NewComputation() {
            std::lock_guard<std::mutex> lock(mutex);
            current_packets_count++;
        }

        bool NeedNewComputation() {
            std::lock_guard<std::mutex> lock(mutex);
            return current_packets_count == computed_packets_count;
        }
    private:
        std::mutex mutex;
        int current_packets_count = {0};
        int computed_packets_count = {0};
        std::vector<Packet> results;
    };


public:
    Analytics() {
        long_computation_cb = std::make_shared<LongComputationControlBlock>(this);
    }

    void Update() {
        std::cout << "update state" << std::endl;

        //if need new long computation
        if (long_computation_cb->NeedNewComputation()) {
            //async call long compute function
            std::cout << "async call" << " " << std::this_thread::get_id() << std::endl;
            Async(long_computation, 100)->Then(long_computation_cb);
            long_computation_cb->NewComputation();
            //process packets
        }
    }

    void PushPacket(Packet pkt) {
        long_computation_cb->PushComputationResult(pkt);
    }
private:
    std::shared_ptr<LongComputationControlBlock>    long_computation_cb;
};


static void on_long_computation_end(Result<Packet> *result, void *analytics_ptr) {
    auto analytics = (Analytics *) analytics_ptr;
    analytics->PushPacket(result->Get());
}


int main() {
    SetupRuntime();

    {
        Analytics analytics;

        for (int i = 0; i < 40 * 5; ++i) {
            analytics.Update();
            std::this_thread::sleep_for(25ms);
        }
    }

    std::this_thread::sleep_for(1000ms);

    Terminate();
    return 0;
}

