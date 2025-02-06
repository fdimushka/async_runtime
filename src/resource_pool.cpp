#include "ar/resource_pool.hpp"
#include <algorithm>
#include <cmath>

using namespace AsyncRuntime;

static int get_chunks_count(size_t size, size_t chunk_size) {
    return (int)ceil((float)size / (float)chunk_size);
}

resource_pool::resource_pool(size_t chunk_sz, size_t nnext_size, size_t nmax_size)
    : pool(chunk_sz, nnext_size, nmax_size), chunk_size(chunk_sz) {
}

resource_pool::~resource_pool() {
    pool.release_memory();
    pool.purge_memory();
}

void *resource_pool::allocate(size_t size) {
    std::lock_guard<std::mutex> const lock(mutex);
    auto *ptr = pool.ordered_malloc(get_chunks_count(size, chunk_size));
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void resource_pool::deallocate(void *ptr, size_t size) {
    std::lock_guard<std::mutex> const lock(mutex);
    pool.ordered_free(ptr, get_chunks_count(size, chunk_size));
}

void resource_pool::deallocate(void *ptr) {
    std::lock_guard<std::mutex> const lock(mutex);
    pool.free(ptr);
}

resource_pools_manager::resource_pools_manager() {
    create_default_resource();
}

resource_pools_manager::~resource_pools_manager() {
    for (auto *pool : pools) {
        delete pool;
    }
}

resource_pool *resource_pools_manager::create_resource(size_t chunk_sz, size_t nnext_size, size_t nmax_size) {
    auto *pool = new resource_pool(chunk_sz, nnext_size, nmax_size);

    std::lock_guard<std::mutex> lock(mutex);
    pools.emplace_back(pool);
    return pool;
}

void resource_pools_manager::delete_resource(resource_pool *pool) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = std::find(pools.begin(), pools.end(), pool);
    if (it != pools.end()) {
        delete *it;
        pools.erase(it);
    }
}

void resource_pools_manager::create_default_resource(size_t chunk_sz, size_t nnext_size, size_t nmax_size) {
    std::lock_guard<std::mutex> lock(mutex);
    default_pool = new resource_pool(chunk_sz, nnext_size, nmax_size);
    pools.push_back(default_pool);
}