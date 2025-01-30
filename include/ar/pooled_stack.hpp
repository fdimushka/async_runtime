#ifndef AR_POOLED_STACK_H
#define AR_POOLED_STACK_H

#include "ar/resource_pool.hpp"

#include <boost/context/continuation.hpp>

namespace AsyncRuntime {
    namespace ctx = boost::context;

    template< typename traitsT >
    class pooled_fixedsize_stack {
    private:
        std::size_t     size_;
        resource_pool *resource;
    public:
        typedef traitsT traits_type;

        pooled_fixedsize_stack( resource_pool *res, std::size_t size = traits_type::default_size() ) BOOST_NOEXCEPT_OR_NOTHROW
        : size_( size),
          resource(res) {}

        ctx::stack_context allocate() {
            void * vp = resource->allocate( size_);
            if ( ! vp) {
                throw std::bad_alloc();
            }

            ctx::stack_context sctx;
            sctx.size = size_;
            sctx.sp = static_cast< char * >( vp) + sctx.size;
            return sctx;
        }

        void deallocate( ctx::stack_context & sctx) BOOST_NOEXCEPT_OR_NOTHROW {
            BOOST_ASSERT( sctx.sp);
            void * vp = static_cast< char * >( sctx.sp) - sctx.size;
            resource->deallocate(vp, size_);
        }
    };
}

#endif //AR_POOLED_STACK_H
