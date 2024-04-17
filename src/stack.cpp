#include "ar/stack.hpp"

#include <mutex>
#include <unistd.h>

extern "C" {
#include <sys/resource.h>
}

using namespace AsyncRuntime;


#if !defined (SIGSTKSZ)
# define SIGSTKSZ (32768) // 32kb minimum allowable stack
# define UDEF_SIGSTKSZ
#endif

#if !defined (MINSIGSTKSZ)
# define MINSIGSTKSZ (131072) // 128kb recommended stack size
# define UDEF_MINSIGSTKSZ
#endif


void pagesize_( std::size_t * size) RNT_NOEXCEPT_OR_NOTHROW {
    * size = ::sysconf( _SC_PAGESIZE);
}


void stacksize_limit_( rlimit * limit) RNT_NOEXCEPT_OR_NOTHROW {
    ::getrlimit( RLIMIT_STACK, limit);
}


std::size_t pagesize() RNT_NOEXCEPT_OR_NOTHROW {
    static std::size_t size = 0;
    static std::once_flag flag;
    std::call_once( flag, pagesize_, & size);
    return size;
}


rlimit stacksize_limit() RNT_NOEXCEPT_OR_NOTHROW {
    static rlimit limit;
    static std::once_flag flag;
    std::call_once( flag, stacksize_limit_, & limit);
    return limit;
}


std::size_t StackTraits::DefaultSize() RNT_NOEXCEPT_OR_NOTHROW
{
    return 1024 * 1024;
}


bool StackTraits::IsUnbounded() RNT_NOEXCEPT_OR_NOTHROW
{
    return RLIM_INFINITY == stacksize_limit().rlim_max;
}


std::size_t StackTraits::MaximumSize() RNT_NOEXCEPT_OR_NOTHROW
{
    assert( ! IsUnbounded() );
    return static_cast< std::size_t >( stacksize_limit().rlim_max);
}


std::size_t StackTraits::MinimumSize() RNT_NOEXCEPT_OR_NOTHROW
{
    return MINSIGSTKSZ;
}


std::size_t StackTraits::PageSize() RNT_NOEXCEPT_OR_NOTHROW
{
    return pagesize();
}


#ifdef UDEF_SIGSTKSZ
# undef SIGSTKSZ
#endif

#ifdef UDEF_MINSIGSTKSZ
# undef MINSIGSTKSZ
#endif