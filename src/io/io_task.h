#ifndef AR_IO_TASK_H
#define AR_IO_TASK_H

#define BOOST_THREAD_PROVIDES_FUTURE 1
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION 1

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/use_future.hpp>
#include <iostream>
#include "ar/task.hpp"
#include "ar/io/tcp.hpp"

namespace AsyncRuntime::IO {
    typedef std::tuple<boost::system::error_code, std::size_t> read_result;

    class io_task : public std::enable_shared_from_this<io_task> {
        typedef promise_t<boost::system::error_code> promise_type;
    public:
        explicit io_task() = default;
        ~io_task() = default;

        void handler(const boost::system::error_code & ec) {
            promise.set_value(ec);
        }

        std::shared_ptr<io_task> get_ptr() { return shared_from_this(); };

        future_t<boost::system::error_code> get_future() { return promise.get_future(); }

        void cancel() {
            promise.set_value({boost::asio::error::timed_out, 0});
        }
    private:
        promise_type promise;
    };

    typedef std::shared_ptr<io_task>   io_task_ptr;

    class write_task : public std::enable_shared_from_this<write_task> {
        typedef promise_t<boost::system::error_code> promise_type;
    public:
        explicit write_task() = default;
        explicit write_task(deadline_timer *deadline) : deadline(deadline) {};
        ~write_task() = default;

        void handler(const boost::system::error_code & ec) {
            try {
                if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                    promise.set_value(ec);
                }
            } catch (...) {
                if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                    promise.set_value(boost::asio::error::eof);
                }
            }
        }

        void handler_deadline() {
            if (deadline != nullptr && deadline->expires_at() <= deadline_timer::traits_type::now()) {
                try {
                    if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                        promise.set_value(boost::asio::error::timed_out);
                    }
                } catch (...) {
                    if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                        promise.set_value(boost::asio::error::eof);
                    }
                }
            }
        }

        std::shared_ptr<write_task> get_ptr() { return this->shared_from_this(); };

        future_t<boost::system::error_code> get_future() { return promise.get_future(); }

        void cancel() {
            if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                promise.set_value(boost::asio::error::eof);
            }
        }
    private:
        deadline_timer *deadline = {nullptr};
        std::atomic_bool resolved = {false};
        promise_type promise;
    };

    class read_task : public std::enable_shared_from_this<read_task> {
        typedef promise_t<read_result> promise_type;
    public:
        explicit read_task() = default;
        explicit read_task(deadline_timer *deadline) : deadline(deadline) {};
        ~read_task() = default;

        void handler(const boost::system::error_code & ec, std::size_t bytes_transferred) {
            try {
                if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                    promise.set_value({ec, bytes_transferred});
                }
            } catch (...) {
                if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                    promise.set_value({boost::asio::error::eof, 0});
                }
            }
        }

        void handler_deadline() {
            if (deadline != nullptr && deadline->expires_at() <= deadline_timer::traits_type::now()) {
                try {
                    if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                        promise.set_value({boost::asio::error::timed_out, 0});
                    }
                } catch (...) {
                    if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                        promise.set_value({boost::asio::error::eof, 0});
                    }
                }
            }
        }

        std::shared_ptr<read_task> get_ptr() { return this->shared_from_this(); };

        future_t<read_result> get_future() { return promise.get_future(); }

        void cancel() {
            if (!std::atomic_exchange_explicit(&resolved, true, std::memory_order_acquire)) {
                promise.set_value({boost::asio::error::timed_out, 0});
            }
        }
    private:
        deadline_timer *deadline = {nullptr};
        std::atomic_bool resolved = {false};
        promise_type promise;
    };

    typedef std::shared_ptr<read_task>   read_task_ptr;
}

#endif //AR_IO_TASK_H
