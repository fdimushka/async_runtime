#ifndef AR_NOTIFIER_H
#define AR_NOTIFIER_H

#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <vector>


namespace AsyncRuntime {

// Modified the event count from Eigen
// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2016 Dmitry Vyukov <dvyukov@google.com>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.


    /**
     * @class Notifier
     * @brief WOrk notifier
     */
    class Notifier {
        friend class runtime;
    public:


        enum WaiterState {
            kNotSignaled,
            kWaiting,
            kSignaled,
        };


        /**
         * @class Waiter
         * @brief
         */
        struct Waiter {
            std::atomic<Waiter*>    next;
            std::mutex              mu;
            std::condition_variable cv;
            uint64_t                epoch;
            WaiterState             state;
        };


        explicit Notifier(size_t N);
        ~Notifier();


        Notifier(const Notifier&) = delete;
        Notifier& operator =(const Notifier&) = delete;
        Notifier(Notifier&&) = delete;
        Notifier& operator =(Notifier&&) = delete;


        /**
         * @brief
         * @param w
         */
        void PrepareWait(Waiter* w);


        /**
         * @brief
         * @param w
         */
        void Wait(Waiter* w);


        /**
         * @brief
         * @param w
         */
        void CancelWait(Waiter* w);


        /**
         * @brief
         * @param all
         */
        void Notify(bool all);


        /**
         * @brief
         * @param all
         */
        void NotifyOne(Waiter* w);


        /**
         * @brief
         * @param n
         */
        void NotifyMany(size_t n);


        /**
         * @brief
         * @return
         */
        [[nodiscard]] size_t Size() const { return _waiters.size(); }


        /**
         * @brief
         * @param index
         * @return
         */
        Waiter* GetWaiter(size_t index) { return &_waiters[index]; }
    private:
        void Park(Waiter* w);
        void UnPark(Waiter* waiters);


        // State_ layout:
        // - low kStackBits is a stack of waiters committed wait.
        // - next kWaiterBits is count of waiters in prewait state.
        // - next kEpochBits is modification counter.
        static const uint64_t kStackBits = 16;
        static const uint64_t kStackMask = (1ull << kStackBits) - 1;
        static const uint64_t kWaiterBits = 16;
        static const uint64_t kWaiterShift = 16;
        static const uint64_t kWaiterMask = ((1ull << kWaiterBits) - 1) << kWaiterShift;
        static const uint64_t kWaiterInc = 1ull << kWaiterBits;
        static const uint64_t kEpochBits = 32;
        static const uint64_t kEpochShift = 32;
        static const uint64_t kEpochMask = ((1ull << kEpochBits) - 1) << kEpochShift;
        static const uint64_t kEpochInc = 1ull << kEpochShift;
        std::atomic<uint64_t> _state;
        std::vector<Notifier::Waiter> _waiters;
    };
}

#endif //AR_NOTIFIER_H
