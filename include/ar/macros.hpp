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


#if defined(RNT_USE_SEGMENTED_STACKS)
    # if ! ( (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) ) ) || \
         (defined(__clang__) && (__clang_major__ > 2 || ( __clang_major__ == 2 && __clang_minor__ > 3) ) ) )
#  error "compiler does not support segmented_stack stacks"
# endif
# define RNT_CONTEXT_SEGMENTS 10
#endif

#if defined(__OpenBSD__)
    // stacks need mmap(2) with MAP_STACK
# define RNT_USE_CONTEXT_USE_MAP_STACK
#endif
}
#endif //AR_MACROS_H
