#include "ar/resource_pool.hpp"
#include <cmath>
#include "ar/stack.hpp"
#include <atomic>
#include <cstddef>
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

resource_pool::storage::storage(size_t chunk_sz, size_t nnext_size, size_t nmax_size)
: pool(chunk_sz, nnext_size, nmax_size), chunk_size(chunk_sz) {
}

resource_pool::storage::~storage() {
    pool.release_memory();
    pool.purge_memory();
}

void *resource_pool::storage::allocate(size_t size) {
    std::lock_guard<std::mutex> const lock(mutex);
    auto *ptr = pool.ordered_malloc(get_chunks_count(size, chunk_size));
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void resource_pool::storage::deallocate(void *ptr, size_t size) {
    std::lock_guard<std::mutex> const lock(mutex);
    pool.free(ptr, get_chunks_count(size, chunk_size));
}

void resource_pool::storage::deallocate(void *ptr) {
    std::lock_guard<std::mutex> const lock(mutex);
    pool.free(ptr);
}

resource_pool::resource_pool(const int64_t id): id(id) {
    add_default_storage();
}

resource_pool::~resource_pool() {
    for (auto storage : storage_map) {
        delete storage.second;
    }
    storage_map.clear();
}

void resource_pool::add_storage(int tag, size_t chunk_sz, size_t nnext_size, size_t nmax_size) {
    storage_map.insert(std::make_pair(tag, new storage(chunk_sz, nnext_size, nmax_size)));
}

void resource_pool::add_default_storage() {
    add_storage(0, 1024, 1024, 0);
}

void *resource_pool::allocate(size_t size, int tag) {
    return storage_map.at(tag)->allocate(size);
}

void resource_pool::deallocate(void *ptr, size_t size, int tag) {
    storage_map.at(tag)->deallocate(ptr, size);
}

void resource_pool::deallocate(void *ptr, int tag) {
    storage_map.at(tag)->deallocate(ptr);
}

resource_pools_manager::resource_pools_manager() {
    create_default_resource();
}

resource_pools_manager::~resource_pools_manager() {
    for (auto pool : pools) {
        delete pool.second;
    }
}

id_type resource_pools_manager::create_resource() {
    id_type id = get_unique_id();
    auto *pool = new resource_pool(id);

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
    std::lock_guard<std::mutex> lock(mutex);
    default_pool = new resource_pool(0);
    pools.insert(std::make_pair(0, default_pool));
}

bool resource_pools_manager::is_from(id_type id) {
    std::lock_guard<std::mutex> lock(mutex);
    return pools.find(id) != pools.end();
}


resource_pool *resource_pools_manager::get_resource(id_type id) {
    std::lock_guard<std::mutex> lock(mutex);
    try {
        return pools.at(id);
    } catch (...) {
        return nullptr;
    }
}