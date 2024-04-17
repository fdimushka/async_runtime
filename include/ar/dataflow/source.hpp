#ifndef AR_DATAFLOW_SOURCE_H
#define AR_DATAFLOW_SOURCE_H

#include "ar/ar.hpp"
#include "ar/dataflow/shared_buffer_factory.hpp"
#include "ar/dataflow/consumer.hpp"
#include "ar/dataflow/port.hpp"
#include "ar/dataflow/notifier.hpp"
#include "ar/dataflow/kernel_events.hpp"
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
        explicit SourcePort(Notifier *notifier, const std::string & name, size_t data_type, SharedBufferType buffer_type, Arguments &&... args)
            : Consumer< T >(name, data_type)
            , notifier(notifier)
            , buffer(SharedBufferFactory::Make< T >(buffer_type, std::forward<Arguments>(args)...)) { };

        template<class ...Arguments>
        explicit SourcePort(Notifier *notifier, const std::string & name, size_t data_type, SharedBufferType buffer_type, const std::function<void(T &)> & deleter, Arguments &&... args)
            : Consumer< T >(name, data_type)
            , notifier(notifier)
            , buffer(SharedBufferFactory::Make< T >(buffer_type, std::forward<Arguments>(args)...))
            , deleter(deleter) { };

        virtual ~SourcePort() {
            Flush();
            delete buffer;
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
    private:
        std::atomic_bool active = {true };
        Notifier *notifier;
        std::function<void(T &)> deleter;
        SharedBuffer< T > *buffer;
    };

    template<class T>
    int SourcePort<T>::Write(T && msg) {
        assert(buffer);
        SharedBufferError error = SharedBufferError::kNO_ERROR;
        error = buffer->Write(std::move(msg));
        if (notifier != nullptr) {
            notifier->Notify((int)KernelEvent::kKERNEL_EVENT_READ_SOURCE);
        }
        return (int)error;
    }

    template<class T>
    int SourcePort<T>::Write(const T & msg) {
        assert(buffer);
        SharedBufferError error = SharedBufferError::kNO_ERROR;
        error = buffer->Write(msg);
        if (notifier != nullptr) {
            notifier->Notify((int)KernelEvent::kKERNEL_EVENT_READ_SOURCE);
        }
        return (int)error;
    }

    template<class T>
    std::optional<T> SourcePort<T>::Read() {
        assert(buffer);
        return buffer->Read();
    }

    template<class T>
    bool SourcePort<T>::TryRead(T & res) {
        assert(buffer);
        return buffer->TryRead(res);
    }

    template<class T>
    bool SourcePort<T>::Empty() {
        assert(buffer);
        return buffer->Empty();
    }

    template<class T>
    void SourcePort<T>::Flush() {
        assert(buffer);
        if (buffer->GetType() == SharedBufferType::kFIFO_BUFFER) {
            while (!buffer->Empty()) {
                auto v = buffer->Read();
                if (v.has_value() && deleter) {
                    deleter(v.value());
                }
            }
        }

        buffer->Flush();
    }

    /**
     * @class Source
     * @brief
     */
    class Source {
        using iterator = std::unordered_map<std::string, std::shared_ptr<Port>>::iterator;
    public:
        explicit Source(Notifier *notifier = nullptr) : notifier(notifier) { };

        template<class T>
        std::shared_ptr<SourcePort<T>> Add( const std::string & port_name, SharedBufferType buffer_type = kFIFO_BUFFER );

        template<class T, class ...Arguments>
        std::shared_ptr<SourcePort<T>> Add( const std::string & port_name, SharedBufferType buffer_type, Arguments &&... args );

        template<class T, class ...Arguments>
        std::shared_ptr<SourcePort<T>> Add( const std::string & port_name, SharedBufferType buffer_type, const std::function<void(T &)> & deleter, Arguments &&... args );

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
        std::unordered_map<std::string, std::shared_ptr<Port>> port_map;
    };

    template< class T >
    std::shared_ptr<SourcePort<T>> Source::Add( const std::string &name, SharedBufferType buffer_type ) {
        std::lock_guard<std::mutex> lock(mutex);
        if (port_map.find(name) != port_map.cend()) {
            throw std::runtime_error("Source port already exists");
        }
        auto source_port = std::make_shared<SourcePort<T>>(notifier, name, typeid(T).hash_code(), buffer_type);
        port_map.template insert(std::make_pair(name, source_port));
        return source_port;
    }

    template<class T, class... Arguments>
    std::shared_ptr<SourcePort<T>> Source::Add(const std::string &name, SharedBufferType buffer_type, const std::function<void(T &)> & deleter, Arguments &&... args) {
        std::lock_guard<std::mutex> lock(mutex);
        if (port_map.find(name) != port_map.cend()) {
            throw std::runtime_error("Source port already exists");
        }
        auto source_port = std::make_shared<SourcePort<T>>(notifier, name, typeid(T).hash_code(), buffer_type, deleter, std::forward<Arguments>(args)...);
        port_map.template insert(std::make_pair(name, source_port));
        return source_port;
    }

    template<class T, class... Arguments>
    std::shared_ptr<SourcePort<T>> Source::Add(const std::string &name, SharedBufferType buffer_type, Arguments &&... args) {
        std::lock_guard<std::mutex> lock(mutex);
        if (port_map.find(name) != port_map.cend()) {
            throw std::runtime_error("Source port already exists");
        }
        auto source_port = std::make_shared<SourcePort<T>>(notifier, name, typeid(T).hash_code(), buffer_type, std::forward<Arguments>(args)...);
        port_map.template insert(std::make_pair(name, source_port));
        return source_port;
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
