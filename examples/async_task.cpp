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
public:
    void Update() {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "update state" << std::endl;

        //if need new long computation
        if (current_packets_count == computed_packets_count) {
            //async call long compute function
            std::cout << "async call" << " " << std::this_thread::get_id() << std::endl;
            Async(long_computation, 100)->Then(on_long_computation_end, this);
            current_packets_count++;
            //process packets
        }
    }

    void PushPacket(Packet pkt) {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "new packet" << " " << std::this_thread::get_id() << std::endl;
        packets.push_back(pkt);
        computed_packets_count++;
    }
private:
    std::mutex mutex;
    int current_packets_count = {0};
    int computed_packets_count = {0};
    std::vector<Packet> packets;
};


static void on_long_computation_end(Result<Packet> *result, void *analytics_ptr) {
    auto analytics = (Analytics *) analytics_ptr;
    analytics->PushPacket(result->Get());
}


int main() {
    SetupRuntime();

    Analytics analytics;

    for (;;) {
        analytics.Update();
        std::this_thread::sleep_for(25ms);
    }

    Terminate();
    return 0;
}

