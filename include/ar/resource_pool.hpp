#ifndef AR_RESOURCE_POOL_H
#define AR_RESOURCE_POOL_H

#include <map>
#include <boost/pool/pool.hpp>
#include <atomic>

namespace AsyncRuntime {

    struct default_allocation_tag { };

    class resource_pool {
    public:
        struct storage {
            std::mutex mutex;
            size_t chunk_size;
            boost::pool<> pool;

            storage(size_t chunk_sz = 128, size_t nnext_size = 1024, size_t nmax_size = 0);
            ~storage();

            void *allocate(size_t size);

            void deallocate(void *ptr, size_t size);

            void deallocate(void *ptr);
        };

        resource_pool(const int64_t id, size_t chunk_sz, size_t nnext_size, size_t nmax_size);
        ~resource_pool();

        resource_pool(const resource_pool & other) = delete;
        resource_pool(resource_pool && other) = delete;

        void add_storage(int tag, size_t chunk_sz, size_t nnext_size, size_t nmax_size);

        void *allocate(size_t size, int tag = 0);

        void deallocate(void *ptr, size_t size, int tag = 0);

        void deallocate(void *ptr, int tag = 0);

        int64_t get_id() const { return id; }
    private:
        void add_default_storage(size_t chunk_sz, size_t nnext_size, size_t nmax_size);

        int64_t id;
        std::map<int, storage*> storage_map;
    };

    class resource_pools_manager {
    public:
        typedef int64_t id_type;

        resource_pools_manager();
        ~resource_pools_manager();

        id_type create_resource(size_t chunk_sz = 128, size_t nnext_size = 1024, size_t nmax_size = 0);

        void delete_resource(id_type id);

        resource_pool *get_resource(id_type id);

        resource_pool *get_default_resource() { return default_pool; }

        bool is_from(id_type id);
    private:
        id_type get_unique_id();
        void create_default_resource(size_t chunk_sz = 128, size_t nnext_size = 1024, size_t nmax_size = 0);

        std::mutex  mutex;
        std::map<id_type, resource_pool*> pools;
        resource_pool *default_pool = nullptr;
        std::atomic<id_type> unique_id = {0};
    };
}

#endif //AR_RESOURCE_POOL_H
