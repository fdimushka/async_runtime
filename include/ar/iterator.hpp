#ifndef AR_ITERATOR_H
#define AR_ITERATOR_H

#include "task.hpp"
#include "coroutine.hpp"


namespace AsyncRuntime {


    template< class CoroutineType >
    class CoroutineIterator {
        typedef typename CoroutineType::YieldType                                   YieldType;
        typedef typename CoroutineType::RetType                                     RetType;

    private:
        CoroutineType *coroutine{nullptr};


        virtual void Fetch() noexcept {
            assert( nullptr != coroutine);
            if ( !( * coroutine).Valid() ) {
                coroutine = nullptr;
                return;
            }
        }


        virtual void Increment() {
            assert( nullptr != coroutine);
            ( * coroutine)();
            Fetch();
        }
    public:
        typedef std::input_iterator_tag iterator_category;
        typedef RetType * pointer;
        typedef RetType & reference;

        typedef pointer   pointer_t;
        typedef reference reference_t;


        CoroutineIterator() noexcept = default;


        explicit CoroutineIterator( CoroutineType * c) noexcept :
                coroutine{ c } {
            Fetch();
        }


        bool operator==( CoroutineIterator const& other) const noexcept {
            return other.coroutine == coroutine;
        }

        bool operator!=( CoroutineIterator const& other) const noexcept {
            return other.coroutine != coroutine;
        }

        CoroutineIterator & operator++() {
            Increment();
            return * this;
        }

        void operator++( int) {
            Increment();
        }

        RetType operator*() const noexcept {
            assert( nullptr != coroutine);
            return coroutine->GetResult()->Get();
        }

        pointer_t operator->() const noexcept {
            assert( nullptr != coroutine);
            return std::addressof( coroutine->GetResult()->Get() );
        }
    };


    template< typename Ret = void, class StackAllocator>
    CoroutineIterator<BaseCoroutine< StackAllocator, Ret>>
    begin( BaseCoroutine< StackAllocator, Ret > & c) {
        return CoroutineIterator<BaseCoroutine< StackAllocator, Ret>>( & c);
    }


    template< typename Ret = void, class StackAllocator>
    CoroutineIterator<BaseCoroutine< StackAllocator, Ret>>
    end( BaseCoroutine< StackAllocator, Ret > &) {
        return CoroutineIterator<BaseCoroutine< StackAllocator, Ret>>();
    }
}

#endif //AR_ITERATOR_H
