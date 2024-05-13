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

//static void on_long_computation_end(Result<Packet> *result, void *analytics_ptr);

class Analytics {
//    class LongComputationControlBlock : public Result<Packet>::CallbackControlBlock {
//    public:
//        explicit LongComputationControlBlock(Analytics *ptr) : Result<Packet>::CallbackControlBlock(on_long_computation_end, ptr) { }
//
//        void PushComputationResult(Packet packet) {
//            std::lock_guard<std::mutex> lock(mutex);
//            results.push_back(packet);
//            computed_packets_count++;
//        }
//
//        void NewComputation() {
//            std::lock_guard<std::mutex> lock(mutex);
//            current_packets_count++;
//        }
//
//        bool NeedNewComputation() {
//            std::lock_guard<std::mutex> lock(mutex);
//            return current_packets_count == computed_packets_count;
//        }
//    private:
//        std::mutex mutex;
//        int current_packets_count = {0};
//        int computed_packets_count = {0};
//        std::vector<Packet> results;
//    };
//

public:
    void Update() {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "update state " << results.size() << std::endl;
        //if need new long computation
        if (current_packets_count == results.size()) {
            //async call long compute function
            std::cout << "async call" << " " << std::this_thread::get_id() << std::endl;
            Async(long_computation, 100).then([this](future_t<Packet> && f){
                auto p = f.get();
                std::lock_guard<std::mutex> lock(mutex);
                std::cout << "finish compute " << " " << std::this_thread::get_id() << std::endl;
                results.push_back(p);
            });
            current_packets_count++;
            //process packets
        }
    }
private:
    std::mutex mutex;
    int current_packets_count = {0};
    std::vector<Packet> results;
};

int main() {
    SetupRuntime();

    {
        Analytics analytics;

        for (int i = 0; i < 40 * 5; ++i) {
            analytics.Update();
            std::this_thread::sleep_for(25ms);
        }
    }

    Terminate();
    return 0;
}

