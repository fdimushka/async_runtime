#include "ar/cpu_helper.hpp"
#include "config.hpp"
#include <math.h>

#ifdef USE_NUMA
#include <numa.h>
#endif

static bool _numa_available() {
    #ifdef USE_NUMA
        return numa_available() >= 0;
    #else
        return false;
    #endif
}


static int _numa_node_of_cpu(int i) {
#ifdef USE_NUMA
    return numa_node_of_cpu(i);
#else
    return 0;
#endif
}


static int _get_numa_nodes_count() {
#ifdef USE_NUMA
    return numa_max_node() + 1;
#else
    return 1;
#endif
}


int AsyncRuntime::SetAffinity(std::thread & thread, const AsyncRuntime::CPU &affinity_cpu) {
#ifdef USE_NUMA
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(affinity_cpu.id, &cpuset);
    return pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
#else
    return 1;
#endif
}


std::vector<AsyncRuntime::CPU> AsyncRuntime::GetCPUs() {
    std::vector<AsyncRuntime::CPU> cpus;
    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        AsyncRuntime::CPU cpu = {};
        cpu.id = i;
        cpu.numa_node_id = _numa_node_of_cpu(i);
        cpus.push_back(cpu);
    }
    return std::move(cpus);
}


std::vector<AsyncRuntime::NumaNode> AsyncRuntime::GetNumaNodes() {
    std::vector<NumaNode> nodes(_get_numa_nodes_count());
    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        AsyncRuntime::CPU cpu = {};
        cpu.id = i;
        cpu.numa_node_id = _numa_node_of_cpu(i);
        nodes[cpu.numa_node_id].id = cpu.numa_node_id;
        nodes[cpu.numa_node_id].cpus.push_back(cpu);
    }
    return nodes;
}


std::vector<AsyncRuntime::NumaNode> AsyncRuntime::GetManualNumaNodes(int count) {
    std::vector<NumaNode> nodes(count);
    int node_cpus_count = static_cast<int>((float)std::thread::hardware_concurrency())/(float)count;
    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        AsyncRuntime::CPU cpu = {};
        cpu.id = i;
        cpu.numa_node_id = floor(static_cast<float>(i)/static_cast<float>(node_cpus_count));
        nodes[cpu.numa_node_id].id = cpu.numa_node_id;
        nodes[cpu.numa_node_id].cpus.push_back(cpu);
    }
    return nodes;
}