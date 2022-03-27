#ifndef AR_MACROS_H
#define AR_MACROS_H

#include <cassert>

namespace AsyncRuntime {

#define RNT_NOEXCEPT noexcept
#define RNT_NOEXCEPT_OR_NOTHROW noexcept
#define RNT_NOEXCEPT_IF(Predicate) noexcept((Predicate))
#define RNT_NOEXCEPT_EXPR(Expression) noexcept((Expression))


#define RNT_ASSERT(expr) assert(expr)
#define RNT_ASSERT_MSG(expr, msg) assert((expr)&&(msg))
}
#endif //AR_MACROS_H
