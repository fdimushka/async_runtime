#include <iostream>
#include <chrono>
#include "ar/ar.hpp"

using namespace AsyncRuntime;
using namespace std::chrono_literals;



int main() {
    SetupRuntime();
    TripleBuffer<int> buffer;

    std::thread producer([&](){
        for(int i = 0; i <= 100; ++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            buffer.write(i);
        }
    });

    std::thread consumer([&](){
        while (true) {
            if(buffer.isUpdate()) {
                int i = buffer.read();
                std::cout << i << std::endl;
                if(i == 100) {
                    break;
                }
            }
        }
    });

    producer.join();
    consumer.join();

    return 0;
}

