#ifndef AR_DUMMY_H
#define AR_DUMMY_H

#include <climits>
#include <cstdint>


namespace AsyncRuntime {
    typedef std::uintptr_t ObjectID;
    typedef std::uintptr_t EntityTag;
#define INVALID_OBJECT_ID UINT_MAX


    class BaseObject {
    public:
        BaseObject() {
            id = reinterpret_cast<std::uintptr_t>(this);
        }
        virtual ~BaseObject() = default;


        virtual /**
         * @brief
         * @return
         */
        ObjectID GetID() const { return id; }

    protected:
        ObjectID id;
    };
}

#endif //AR_DUMMY_H
