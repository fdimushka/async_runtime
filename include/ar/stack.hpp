#ifndef AR_STACK_H
#define AR_STACK_H

#include <cstddef>
#include <new>
#include <cstring>
#include <memory>
#include <cstdlib>

#include <boost/context/continuation.hpp>

namespace AsyncRuntime {
    namespace ctx = boost::context;

    template< typename traitsT >
    class basic_fixedsize_stack {
    private:
        std::size_t     size_;

    public:
        typedef traitsT traits_type;

        basic_fixedsize_stack( std::size_t size = traits_type::default_size() ) BOOST_NOEXCEPT_OR_NOTHROW :
                size_( size) {
                }

        ctx::stack_context allocate() {
#if defined(BOOST_CONTEXT_USE_MAP_STACK)
            void * vp = ::mmap( 0, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_STACK, -1, 0);
        if ( vp == MAP_FAILED) {
            throw std::bad_alloc();
        }
#else
            void * vp = std::malloc( size_);
            if ( ! vp) {
                throw std::bad_alloc();
            }
#endif
            ctx::stack_context sctx;
            sctx.size = size_;
            sctx.sp = static_cast< char * >( vp) + sctx.size;
#if defined(BOOST_USE_VALGRIND)
            sctx.valgrind_stack_id = VALGRIND_STACK_REGISTER( sctx.sp, vp);
#endif
            return sctx;
        }

        void deallocate( ctx::stack_context & sctx) BOOST_NOEXCEPT_OR_NOTHROW {
            BOOST_ASSERT( sctx.sp);

#if defined(BOOST_USE_VALGRIND)
            VALGRIND_STACK_DEREGISTER( sctx.valgrind_stack_id);
#endif
            void * vp = static_cast< char * >( sctx.sp) - sctx.size;
#if defined(BOOST_CONTEXT_USE_MAP_STACK)
            ::munmap( vp, sctx.size);
#else
            std::free( vp);
#endif
        }
    };
}

#endif //AR_STACK_H
