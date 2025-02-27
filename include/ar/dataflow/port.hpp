#ifndef AR_DATAFLOW_PORT_H
#define AR_DATAFLOW_PORT_H

#include "ar/ar.hpp"
#include <typeinfo>
#include <set>

namespace AsyncRuntime::Dataflow {
    /**
     * @struct SinkUser
     * @brief
     */
    struct PortUser { };


    /**
     * @class Port
     * @brief
     */
    class Port {
        using PortSet = std::set<const PortUser*, std::less<const PortUser*>, Allocator<const PortUser*>>;
    public:
        Port(const std::string & name, size_t data_type);
        Port(resource_pool *resource, const std::string & name, size_t data_type);
        virtual ~Port() = default;

        const std::string & GetName() const { return name; }
        virtual void Subscribe(const PortUser *user);
        virtual void Unsubscribe(const PortUser *user);
        virtual void UnsubscribeAll();
        bool SubscribersEmpty();
        virtual void Flush() = 0;
        virtual void Activate() {  };
        virtual void Deactivate() {  };
    private:
        std::string name;
        PortSet subscribers;
        std::atomic_size_t subscribers_count = {0};
        size_t data_type;
    };
}

#endif //AR_DATAFLOW_PORT_H
