#include "ar/resource_pool.hpp"
#include <cmath>
#include "ar/stack.hpp"
#include <atomic>
#include <iostream>

using namespace AsyncRuntime;
namespace ctx = boost::context;
using id_type = resource_pools_manager::id_type;

static int get_chunks_count(size_t size, size_t chunk_size) {
    return (int)ceil((float)size / (float)chunk_size);
}

static id_type get_unique_id() {
    static std::atomic<id_type> id = {1};
    return ++id;
}

resource_pool::resource_pool(const int64_t id,
              const size_t chunk_sz,
              const size_t nnext_size,
              const size_t nmax_size)
        : id(id)
        , chunk_size(chunk_sz)
        , storage(chunk_sz, nnext_size, nmax_size) {
}

resource_pool::~resource_pool() {
    storage.purge_memory();
}

void *resource_pool::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    auto *ptr = storage.ordered_malloc(get_chunks_count(size, chunk_size));
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void resource_pool::deallocate(void *ptr, size_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    storage.free(ptr, get_chunks_count(size, chunk_size));
}

void resource_pool::deallocate(void *ptr) {
    std::lock_guard<std::mutex> lock(mutex);
    storage.free(ptr);
}

resource_pools_manager::resource_pools_manager() {
    create_default_resource();
}

resource_pools_manager::~resource_pools_manager() {
    for (auto pool : pools) {
        delete pool.second;
    }
};

void resource_pools_manager::add_thread_ids(const std::vector<std::thread::id> & ids) {
    for (const auto id : ids) {
        current_resources.insert(std::make_pair(id, nullptr));
    }
}

id_type resource_pools_manager::create_resource(size_t chunk_sz, size_t nnext_size, size_t nmax_size) {
    id_type id = get_unique_id();
    auto *pool = new resource_pool{id, chunk_sz, nnext_size, nmax_size};

    std::lock_guard<std::mutex> lock(mutex);
    pools.insert(std::make_pair(id, pool));
    return id;
}

void resource_pools_manager::delete_resource(id_type id) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = pools.find(id);
    if (it != pools.end()) {
        delete it->second;
        pools.erase(it);
    }
}

void resource_pools_manager::create_default_resource() {
    default_pool = new resource_pool{0, 1024*1024, 1024};
    pools.insert(std::make_pair(0, default_pool));
}

bool resource_pools_manager::is_from(id_type id) {
    std::lock_guard<std::mutex> lock(mutex);
    return pools.find(id) != pools.end();
}

void resource_pools_manager::set_current_resource(resource_pool * resource) {
    current_resources[std::this_thread::get_id()] = resource;
}

resource_pool *resource_pools_manager::get_current_resource() {
    try {
        auto *pool = current_resources.at(std::this_thread::get_id());
        if (pool != nullptr) {
            return pool;
        }
        return nullptr;
    }catch (...) {
        return nullptr;
    }
}

resource_pool *resource_pools_manager::get_resource(id_type id) {
    std::lock_guard<std::mutex> lock(mutex);
    try {
        return pools.at(id);
    } catch (...) {
        return nullptr;
    }
}
