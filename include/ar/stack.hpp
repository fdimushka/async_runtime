#ifndef AR_STACK_H
#define AR_STACK_H

#include <cstddef>
#include <new>
#include <cstring>
#include <memory>
#include <cstdlib>

#include "ar/macros.hpp"

# if defined(RNT_USE_SEGMENTED_STACKS)
extern "C" {
void *__splitstack_makecontext( std::size_t,
                                void * [RNT_CONTEXT_SEGMENTS],
                                std::size_t *);

void __splitstack_releasecontext( void * [RNT_CONTEXT_SEGMENTS]);

void __splitstack_resetcontext( void * [RNT_CONTEXT_SEGMENTS]);

void __splitstack_block_signals_context( void * [RNT_CONTEXT_SEGMENTS],
                                         int * new_value, int * old_value);
}
# endif

namespace AsyncRuntime {

    /**
     * @struct StackTraits
     * @brief
     */
    struct  StackTraits
    {
        static bool IsUnbounded() RNT_NOEXCEPT_OR_NOTHROW;
        static std::size_t PageSize() RNT_NOEXCEPT_OR_NOTHROW;
        static std::size_t DefaultSize() RNT_NOEXCEPT_OR_NOTHROW;
        static std::size_t MinimumSize() RNT_NOEXCEPT_OR_NOTHROW;
        static std::size_t MaximumSize() RNT_NOEXCEPT_OR_NOTHROW;
    };


    /**
     * @struct
     * @brief
     */
    struct StackContext {
        StackContext() :sp(0), size(0)
# if defined(RNT_USE_SEGMENTED_STACKS)
        , segments_ctx()
# endif
        {};

        std::size_t              size;
        void                    *sp;

# if defined(RNT_USE_SEGMENTED_STACKS)
        typedef void *  segments_context[RNT_CONTEXT_SEGMENTS];
# endif

# if defined(RNT_USE_SEGMENTED_STACKS)
        segments_context        segments_ctx{};
# endif
    };

    struct Preallocated {
        void        *   sp;
        std::size_t     size;
        StackContext   sctx;

        Preallocated( void * sp_, std::size_t size_, StackContext sctx_) noexcept :
                sp( sp_), size( size_), sctx( sctx_) {
        }
    };

    /**
     * @class BasicFixedSizeStack<>
     * @brief
     * @tparam TraitsT
     */
    template< typename TraitsT >
    class BasicFixedSizeStack {
    private:
        std::size_t     size_;

    public:
        typedef TraitsT traits_type;


        explicit BasicFixedSizeStack( std::size_t size = traits_type::DefaultSize() ) RNT_NOEXCEPT_OR_NOTHROW :
                size_( size) {
        }


        virtual StackContext Allocate() {
            void * vp = std::malloc( size_);
            if ( ! vp) {
                throw std::bad_alloc();
            }

            //std::memset(vp, 0, size_);

            StackContext sctx;
            sctx.size = size_;
            sctx.sp = static_cast< char * >( vp) + sctx.size;
            return sctx;
        }


        virtual void Deallocate( StackContext & sctx) RNT_NOEXCEPT_OR_NOTHROW {
            assert( sctx.sp);
            void * vp = static_cast< char * >( sctx.sp) - sctx.size;
            std::free( vp);
        }
    };

# if defined(RNT_USE_SEGMENTED_STACKS)
    /**
     * @brief
     * @tparam TraitsT
     */
    template< typename TraitsT >
    class BasicSegmentedStack {
    private:
        std::size_t     size_;

    public:
        typedef TraitsT traits_type;


        explicit BasicSegmentedStack( std::size_t size = traits_type::DefaultSize() ) RNT_NOEXCEPT_OR_NOTHROW :
                size_( size) {
        }


        virtual StackContext Allocate() {
            AsyncRuntime::StackContext sctx;
            void * vp = __splitstack_makecontext( size_, sctx.segments_ctx, & sctx.size);
            if ( ! vp) throw std::bad_alloc();

            // sctx.size is already filled by __splitstack_makecontext
            sctx.sp = static_cast< char * >( vp) + sctx.size;

            int off = 0;

            __splitstack_block_signals_context( sctx.segments_ctx, & off, 0);

            return sctx;
        }


        virtual void Deallocate( StackContext & sctx) RNT_NOEXCEPT_OR_NOTHROW {
            __splitstack_releasecontext( sctx.segments_ctx);
        }
    };
    typedef BasicSegmentedStack< StackTraits > SegmentedStack;
# endif

    typedef BasicFixedSizeStack< StackTraits > FixedSizeStack;
}

#endif //AR_STACK_H
