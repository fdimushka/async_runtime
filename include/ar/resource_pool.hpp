#ifndef AR_RESOURCE_POOL_H
#define AR_RESOURCE_POOL_H

#include <iterator>
#include <type_traits>
#include <map>
#include <vector>
#include <thread>
#include <boost/pool/simple_segregated_storage.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>

namespace AsyncRuntime {

    class resource_pool {
    public:
        explicit resource_pool(int64_t id,
                               size_t chunk_sz,
                               size_t nnext_size = 32,
                               size_t nmax_size = 0);
        ~resource_pool();

        resource_pool(const resource_pool & other) = delete;
        resource_pool(resource_pool && other) = delete;

        void *allocate(size_t size);

        void deallocate(void *ptr, size_t size);

        void deallocate(void *ptr);
    private:
        int64_t id;
        size_t chunk_size;
        std::mutex mutex;
        std::vector<void*> blocks;
        boost::pool<> storage;
    };

    class resource_pools_manager {
    public:
        typedef int64_t id_type;

        resource_pools_manager();
        ~resource_pools_manager();

        void add_thread_ids(const std::vector<std::thread::id> & ids);

        id_type create_resource(size_t chunk_sz = 1024*1024, size_t nnext_size = 1024, size_t nmax_size = 0);

        void delete_resource(id_type id);

        resource_pool *get_resource(id_type id);

        resource_pool *get_current_resource();

        resource_pool *get_default_resource() { return default_pool; }

        void set_current_resource(resource_pool * resource);

        bool is_from(id_type id);
    private:
        void create_default_resource();

        std::mutex  mutex;
        std::map<id_type, resource_pool*> pools;
        std::map<std::thread::id, resource_pool *> current_resources;
        resource_pool *default_pool;
    };
}

#endif //AR_RESOURCE_POOL_H
