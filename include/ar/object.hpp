#ifndef AR_DUMMY_H
#define AR_DUMMY_H

#include <iostream>
#include <functional>
#include <vector>
#include <queue>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <climits>
#include <atomic>
#include <memory>


namespace AsyncRuntime {
    typedef std::uintptr_t ObjectID;


    class BaseObject {
    public:
        BaseObject() { id = reinterpret_cast<std::uintptr_t>(this); }
        virtual ~BaseObject() = default;


        /**
         * @brief
         * @return
         */
        ObjectID GetID() const { return id; }

    protected:
        ObjectID id;
    };
}

#endif //AR_DUMMY_H
