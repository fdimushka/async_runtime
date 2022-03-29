#ifndef AR_STREAM_HPP
#define AR_STREAM_HPP


#include "ar/task.hpp"


namespace AsyncRuntime {
    template<typename T>
    class Stream {
        virtual void Write() = 0;
        virtual std::optional<T>  Read() = 0;
    };
}

#endif //AR_STREAM_HPP
