#ifndef AR_DATAFLOW_SOURCE_H
#define AR_DATAFLOW_SOURCE_H

#include "ar/ar.hpp"
#include "ar/dataflow/shared_buffer_factory.hpp"
#include "ar/dataflow/consumer.hpp"
#include "ar/dataflow/port.hpp"
#include "ar/dataflow/notifier.hpp"
#include "ar/dataflow/kernel_events.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include <typeinfo>

namespace AsyncRuntime::Dataflow {

    /**
     * @class SourcePort < T >
     * @tparam T
     */
    template < class T >
    class SourcePort : public Consumer< T > {
    public:
        template<class ...Arguments>
        SourcePort(Notifier *notifier, const std::string & name, size_t data_type, SharedBufferType buffer_type, int capacity = 100)
            : Consumer< T >(name, data_type)
            , notifier(notifier)
            , queue(capacity) { };

        virtual ~SourcePort() {
            Flush();
        }

        int Write(T && msg) final;
        int Write(const T & msg) final;
        virtual std::optional<T> Read();
        bool TryRead(T & res);
        bool Empty();
        void Flush() final;
        void Activate() final { active.store(true, std::memory_order_relaxed ); };
        void Deactivate() final  { active.store(false, std::memory_order_relaxed ); };
        virtual bool IsActive() { return active.load(std::memory_order_relaxed); }
        void SetSkipCounter(const std::shared_ptr<Mon::Counter> & counter) { skip_counter = counter; }
    private:
        std::atomic_bool active = {true };
        Notifier *notifier;
        boost::lockfree::spsc_queue<T, boost::lockfree::fixed_sized<true>> queue;
        std::shared_ptr<Mon::Counter> skip_counter;
    };

    template<class T>
    int SourcePort<T>::Write(T && msg) {
        SharedBufferError error = SharedBufferError::kNO_ERROR;
        if (!queue.push(std::move(msg))) {
            error = SharedBufferError::kBUFFER_OVERFLOW;
            if (skip_counter) {
                skip_counter->Increment();
            }
        }

        if (notifier != nullptr) {
            notifier->Notify((int)KernelEvent::kKERNEL_EVENT_READ_SOURCE);
        }
        return (int)error;
    }

    template<class T>
    int SourcePort<T>::Write(const T & msg) {
        SharedBufferError error = SharedBufferError::kNO_ERROR;
        if (!queue.push(msg)) {
            error = SharedBufferError::kBUFFER_OVERFLOW;
            if (skip_counter) {
                skip_counter->Increment();
            }
        }

        if (notifier != nullptr) {
            notifier->Notify((int)KernelEvent::kKERNEL_EVENT_READ_SOURCE);
        }
        return (int)error;
    }

    template<class T>
    std::optional<T> SourcePort<T>::Read() {
        T msg;
        if (queue.pop(msg)) {
            return msg;
        } else {
            return std::nullopt;
        }
    }

    template<class T>
    bool SourcePort<T>::TryRead(T & res) {
        return queue.pop(res);
    }

    template<class T>
    bool SourcePort<T>::Empty() {
        return queue.empty();
    }

    template<class T>
    void SourcePort<T>::Flush() {
        while (!queue.empty()) {
            T msg;
            if (!queue.pop(msg)) {
                break;
            }
        }
    }

    /**
     * @class Source
     * @brief
     */
    class Source {
        using iterator = std::unordered_map<std::string, std::shared_ptr<Port>>::iterator;
        using PortMapAllocator = Allocator<std::pair<const std::string, std::shared_ptr<Port>>>;
        using PortMap = std::unordered_map<std::string, std::shared_ptr<Port>, std::hash<std::string>, std::equal_to<std::string>, PortMapAllocator>;
    public:
        explicit Source(Notifier *notifier = nullptr) : notifier(notifier) { };
        Source(resource_pool *res, Notifier *notifier = nullptr)
        : notifier(notifier)
        , resource(res)
        , port_map(PortMapAllocator{res}) { };

        template<class T>
        std::shared_ptr<SourcePort<T>> Add( const std::string & port_name, SharedBufferType buffer_type = kFIFO_BUFFER );

        template<class T, class ...Arguments>
        std::shared_ptr<SourcePort<T>> Add( const std::string & port_name, SharedBufferType buffer_type, Arguments &&... args );

        template < class T >
        std::shared_ptr<SourcePort<T>> At( const std::string & port_name );

        template < class T >
        std::shared_ptr<SourcePort<T>> operator [] ( const std::string & port_name );

        void Flush();

        iterator Begin() { return port_map.begin(); };
        iterator End() { return port_map.end(); };

        void Activate();
        void Deactivate();
    private:
        std::mutex mutex;
        Notifier *notifier;
        resource_pool *resource = nullptr;
        PortMap port_map;
    };

    template< class T >
    std::shared_ptr<SourcePort<T>> Source::Add( const std::string &name, SharedBufferType buffer_type ) {
        std::lock_guard<std::mutex> lock(mutex);
        if (port_map.find(name) != port_map.cend()) {
            throw std::runtime_error("Source port already exists");
        }

        if (resource != nullptr) {
            auto source_port = make_shared_ptr<SourcePort<T>>(resource, notifier, name, typeid(T).hash_code(), buffer_type);
            port_map.template insert(std::make_pair(name, source_port));
            return source_port;
        } else {
            auto source_port = std::make_shared<SourcePort<T>>(notifier, name, typeid(T).hash_code(), buffer_type);
            port_map.template insert(std::make_pair(name, source_port));
            return source_port;
        }
    }

    template<class T, class... Arguments>
    std::shared_ptr<SourcePort<T>> Source::Add(const std::string &name, SharedBufferType buffer_type, Arguments &&... args) {
        std::lock_guard<std::mutex> lock(mutex);
        if (port_map.find(name) != port_map.cend()) {
            throw std::runtime_error("Source port already exists");
        }

        if (resource != nullptr) {
            auto source_port = make_shared_ptr<SourcePort<T>>(resource, notifier, name, typeid(T).hash_code(), buffer_type, std::forward<Arguments>(args)...);
            port_map.template insert(std::make_pair(name, source_port));
            return source_port;
        } else {
            auto source_port = std::make_shared<SourcePort<T>>(notifier, name, typeid(T).hash_code(), buffer_type, std::forward<Arguments>(args)...);
            port_map.template insert(std::make_pair(name, source_port));
            return source_port;
        }
    }

    template<class T>
    std::shared_ptr<SourcePort<T>> Source::At(const std::string &port_name) {
        std::lock_guard<std::mutex> lock(mutex);
        const auto &it = port_map.find(port_name);
        if (it == port_map.cend()) {
            throw std::runtime_error("Source port not found");
        }
        return std::static_pointer_cast<SourcePort<T>>(it->second);
    }

    template<class T>
    std::shared_ptr<SourcePort<T>> Source::operator[](const std::string &port_name) {
        std::lock_guard<std::mutex> lock(mutex);
        const auto &it = port_map.find(port_name);
        if (it == port_map.cend()) {
            throw std::runtime_error("Source port not found");
        }
        return std::static_pointer_cast<SourcePort<T>>(it->second);
    }
}

#endif //AR_DATAFLOW_SOURCE_H
