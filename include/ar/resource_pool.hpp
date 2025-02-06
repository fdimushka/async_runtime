#ifndef AR_RESOURCE_POOL_H
#define AR_RESOURCE_POOL_H

#include <vector>
#include <boost/pool/pool.hpp>

namespace AsyncRuntime {

    struct default_allocation_tag { };

    class resource_pool {
    public:
        resource_pool(size_t chunk_sz = 128, size_t nnext_size = 1024, size_t nmax_size = 0);
        ~resource_pool();

        resource_pool(const resource_pool & other) = delete;
        resource_pool(resource_pool && other) = delete;

        resource_pool& operator=(const resource_pool & other) = delete;
        resource_pool& operator=(resource_pool && other) = delete;

        void *allocate(size_t size);

        void deallocate(void *ptr, size_t size);

        void deallocate(void *ptr);
    private:

        std::mutex mutex;
        size_t chunk_size;
        boost::pool<> pool;
    };

    class resource_pools_manager {
    public:

        resource_pools_manager();
        ~resource_pools_manager();

        resource_pool *create_resource(size_t chunk_sz = 128, size_t nnext_size = 1024, size_t nmax_size = 0);

        void delete_resource(resource_pool *pool);

        resource_pool *get_default_resource() { return default_pool; }

    private:
        void create_default_resource(size_t chunk_sz = 128, size_t nnext_size = 1024, size_t nmax_size = 0);

        std::mutex mutex;
        std::vector<resource_pool *> pools;
        resource_pool *default_pool = nullptr;
    };

    extern resource_pool *CreateResource(size_t chunk_sz, size_t nnext_size, size_t nmax_size);
    extern void DeleteResource(resource_pool *pool);

    struct resource_pool_deleter {
        void operator()(resource_pool *pool) const {
            AsyncRuntime::DeleteResource(pool);
        }
    };
}

#endif //AR_RESOURCE_POOL_H
