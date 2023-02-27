#ifndef AR_STACK_H
#define AR_STACK_H

#include "ar/helper.hpp"

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
        StackContext() :sp(nullptr), begin(nullptr), size(0) {};
        std::size_t              size;
        void                    *sp;
        void                    *begin;
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
            StackContext sctx;
            sctx.size = size_;
            sctx.sp = static_cast< char * >( vp) + sctx.size;
            sctx.begin = sctx.sp;
            return sctx;
        }


        virtual void Deallocate( StackContext & sctx) RNT_NOEXCEPT_OR_NOTHROW {
            assert( sctx.sp);
            void * vp = static_cast< char * >( sctx.begin) - sctx.size;
            std::free( vp);
        }
    };

    typedef BasicFixedSizeStack< StackTraits > FixedSizeStack;
}

#endif //AR_STACK_H
