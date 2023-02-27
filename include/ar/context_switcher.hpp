#ifndef AR_CONTEXT_SWITCHER_HPP
#define AR_CONTEXT_SWITCHER_HPP

#include <iostream>

typedef void*   fcontext_t;

struct transfer_t {
    fcontext_t  fctx;
    void    *   data;
};
# define AR_CONTEXT_DECL

#if (defined(i386) || defined(__i386__) || defined(__i386) \
     || defined(__i486__) || defined(__i586__) || defined(__i686__) \
     || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) \
     || defined(__I86__) || defined(__INTEL__) || defined(__IA32__) \
     || defined(_M_IX86) || defined(_I86_)) && defined(BOOST_WINDOWS)
# define AR_CALLDECL __cdecl
#else
# define AR_CALLDECL
#endif

extern "C" AR_CONTEXT_DECL
        transfer_t AR_CALLDECL jump_fcontext( fcontext_t const to, void * vp);
extern "C" AR_CONTEXT_DECL
        fcontext_t AR_CALLDECL make_fcontext( void * sp, std::size_t size, void (* fn)( transfer_t) );

extern "C" AR_CONTEXT_DECL
        transfer_t AR_CALLDECL ontop_fcontext( fcontext_t const to, void * vp, transfer_t (* fn)( transfer_t) );


namespace AsyncRuntime {


    namespace Context {
        inline transfer_t Jump(fcontext_t const to, void *vp) {
            return jump_fcontext(to, vp);
        }


        inline fcontext_t  Make(void *sp, std::size_t size, void (*fn)(transfer_t)) {
            return make_fcontext(sp, size, fn);
        }


        inline transfer_t OnTop(fcontext_t const to, void *vp, transfer_t (*fn)(transfer_t)) {
            return ontop_fcontext(to, vp, fn);
        }
    }
}

#endif //AR_CONTEXT_SWITCHER_HPP
