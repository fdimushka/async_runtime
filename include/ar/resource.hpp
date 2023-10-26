#ifndef AR_RESOURCE_H
#define AR_RESOURCE_H

#include <iostream>
#include <cstdint>

namespace AsyncRuntime {
    typedef std::uintptr_t ObjectID;

    struct Resource {
        size_t id;

        explicit Resource(const std::string& name);
    };
}

#endif //AR_RESOURCE_H
