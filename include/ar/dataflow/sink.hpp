#ifndef AR_DATAFLOW_SINK_H
#define AR_DATAFLOW_SINK_H

#include "ar/ar.hpp"
#include "ar/dataflow/consumer.hpp"
#include "ar/dataflow/port.hpp"
#include "ar/dataflow/notifier.hpp"
#include "ar/dataflow/kernel_events.hpp"

namespace AsyncRuntime::Dataflow {
    /**
     * @class SinkPort
     * @tparam T
     * @brief
     */
    template<typename T>
    class SinkPort : public Port {
        using consumer_iterator = typename std::list<std::shared_ptr<Consumer< T >>>::iterator;
    public:
        SinkPort(const std::string & name, size_t data_type, Notifier *notifier)
            : Port(name, data_type)
            , notifier(notifier)
            , last_msg_ts(0) { };

        SinkPort(const std::string & name, size_t data_type, Notifier *notifier, const std::function<void(T &)> & deleter)
                : Port(name, data_type)
                , notifier(notifier)
                , last_msg_ts(0)
                , deleter(deleter) { };

        ~SinkPort() override = default;

        void Send(T && msg);
        void Send(const T & msg);
        bool Send(const consumer_iterator & it, T && msg);
        bool Send(const consumer_iterator & it, const T & msg);
        bool IsActive(const consumer_iterator & it);

        void Connect(const std::shared_ptr<Consumer<T >> &consumer);
        void Disconnect(const std::shared_ptr<Consumer<T >> &consumer);
        void DisconnectAll();
        void Subscribe(const PortUser *user) override;
        void Unsubscribe(const PortUser *user) override;
        void Flush() final { };
        int ConsumersCount();
        int64_t GetLastMsgTs() const { return last_msg_ts; }

        consumer_iterator Begin() { return consumers.begin(); };
        consumer_iterator End() { return consumers.end(); };
    protected:
        int64_t last_msg_ts;
        Notifier *notifier;
        std::function<void(T &)> deleter;
        std::list<std::shared_ptr<Consumer< T >>> consumers;
    };

    template<typename T>
    void SinkPort<T>::Connect(const std::shared_ptr<Consumer<T >> &consumer) {
        auto it = std::find_if(consumers.begin(), consumers.end(), [consumer](std::shared_ptr<Consumer<T >> p) {
            if (consumer && p)
                return consumer == p;
            return false;
        });
        if (it != consumers.end()) {
            throw std::runtime_error("Consumer already exist");
        }
        consumers.push_back(consumer);
    }

    template<typename T>
    void SinkPort<T>::Disconnect(const std::shared_ptr<Consumer<T >> &consumer) {
        consumers.remove_if([consumer](std::shared_ptr<Consumer<T >> p) {
            if (consumer && p)
                return consumer == p;
            return false;
        });
    }

    template<typename T>
    void SinkPort<T>::DisconnectAll() {
        consumers.clear();
    }

    template<typename T>
    bool SinkPort<T>::Send(const consumer_iterator & it, T && msg) {
        if ((*it)->Write(std::move(msg)) == 0) {
            return true;
        }
        return false;
    }

    template<typename T>
    bool SinkPort<T>::Send(const consumer_iterator & it, const T & msg) {
        if ((*it)->Write(msg) == 0) {
            return true;
        }
        return false;
    }

    template<typename T>
    bool SinkPort<T>::IsActive(const consumer_iterator & it) {
        return (*it)->IsActive();
    }

    template<typename T>
    void SinkPort<T>::Send(const T & msg) {
        last_msg_ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        typename std::list<std::shared_ptr<Consumer<T >>>::iterator it = consumers.begin();
        while (it != consumers.end()) {
            if ((*it)->IsActive()) {
                (*it)->Write(msg);
            }
            ++it;
        }
    }

    template<typename T>
    void SinkPort<T>::Send(T && msg) {
        last_msg_ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        typename std::list<std::shared_ptr<Consumer<T >>>::iterator it = consumers.begin();
        while (it != consumers.end()) {
            if ((*it)->IsActive()) {
                (*it)->Write(std::move(msg));
            }
            ++it;
        }
    }

    template<typename T>
    int SinkPort<T>::ConsumersCount() {
        return consumers.size();
    }

    template<typename T>
    void SinkPort<T>::Subscribe(const PortUser *user) {
        Port::Subscribe(user);
        if (notifier) {
            notifier->Notify((int)KernelEvent::kKERNEL_EVENT_SINK_SUBSCRIPTION);
        }
    }

    template<typename T>
    void SinkPort<T>::Unsubscribe(const PortUser *user) {
        bool has_notify = !Port::SubscribersEmpty();
        Port::Unsubscribe(user);
        if (has_notify && notifier) {
            notifier->Notify((int)KernelEvent::kKERNEL_EVENT_SINK_UNSUBSCRIPTION);
        }
    }

    /**
     * @class Sink
     * @brief
     */
    class Sink {
        using iterator = std::unordered_map<std::string, std::shared_ptr<Port>>::iterator;
    public:
        explicit Sink(Notifier *notifier = nullptr) : notifier(notifier) { };
        virtual ~Sink() = default;

        template<class T>
        std::shared_ptr<SinkPort<T>> Add( const std::string & port_name );

        template<class T>
        std::shared_ptr<SinkPort<T>> Add( const std::string & port_name, const std::function<void(T &)> & deleter );

        template < class T >
        std::shared_ptr<SinkPort<T>> At( const std::string & port_name );

        std::shared_ptr<Port> At( const std::string & port_name );

        std::shared_ptr<Port> operator [] ( const std::string & port_name );

        void Subscribe( const std::string & port_name, const PortUser *user );
        void Unsubscribe( const std::string & port_name, const PortUser *user );
        void UnsubscribeAll();
        void DisconnectAll();
        bool SubscribersEmpty();

        iterator Begin() { return port_map.begin(); };
        iterator End() { return port_map.end(); };
    private:
        std::mutex mutex;
        Notifier *notifier;
        std::unordered_map<std::string, std::shared_ptr<Port>> port_map;
    };

    template< class T >
    std::shared_ptr<SinkPort<T>> Sink::Add( const std::string &name ) {
        std::lock_guard<std::mutex> lock(mutex);
        if (port_map.find(name) != port_map.cend()) {
            throw std::runtime_error("Source port already exists");
        }
        auto port = std::make_shared<SinkPort<T>>(name, typeid(T).hash_code(), notifier);
        port_map.template insert(std::make_pair(name, port));
        return port;
    }

    template< class T >
    std::shared_ptr<SinkPort<T>> Sink::Add( const std::string &name, const std::function<void(T &)> & deleter ) {
        std::lock_guard<std::mutex> lock(mutex);
        if (port_map.find(name) != port_map.cend()) {
            throw std::runtime_error("Source port already exists");
        }
        auto port = std::make_shared<SinkPort<T>>(name, typeid(T).hash_code(), notifier, deleter);
        port_map.template insert(std::make_pair(name, port));
        return port;
    }

    template<class T>
    std::shared_ptr<SinkPort<T>> Sink::At(const std::string &port_name) {
        return std::static_pointer_cast<SinkPort<T>>(At(port_name));
    }
}

#endif //AR_DATAFLOW_SINK_H
