//#include <cpuid.h>
#include <thread>
#include "ar/os.hpp"


std::string AsyncRuntime::OS::GetCPUInfo()
{
    std::string info = "";
    //info.resize(49);
    //uint *cpu_info = reinterpret_cast<uint*>(info.data());
    //for (uint i=0; i<3; i++)
        //__cpuid(0x80000002+i, cpu_info[i*4+0], cpu_info[i*4+1], cpu_info[i*4+2], cpu_info[i*4+3]);
    //info.assign(info.data());
    info += " cores " + std::to_string(std::thread::hardware_concurrency());
    return info;
}
