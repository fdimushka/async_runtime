#ifndef AR_FUNCTION_HPP
#define AR_FUNCTION_HPP

#include "ar/coroutine.hpp"

#define ASYNC(FUNC_NAME, ...) FUNC_NAME(AsyncRuntime::CoroutineHandler *coroutine_handler, __VA_ARGS__)

#endif //AR_FUNCTION_HPP
