#ifndef AR_CONTEXT_SWITCHER_HPP
#define AR_CONTEXT_SWITCHER_HPP

#include <iostream>
#include <boost/context/detail/fcontext.hpp>

namespace AsyncRuntime {
    typedef boost::context::detail::fcontext_t fcontext_t;
    typedef boost::context::detail::transfer_t transfer_t;


    namespace Context {
        inline transfer_t Jump(fcontext_t const to, void *vp) {
            return boost::context::detail::jump_fcontext(to, vp);
        }


        inline fcontext_t  Make(void *sp, std::size_t size, void (*fn)(transfer_t)) {
            return boost::context::detail::make_fcontext(sp, size, fn);
        }


        inline transfer_t OnTop(fcontext_t const to, void *vp, transfer_t (*fn)(transfer_t)) {
            return boost::context::detail::ontop_fcontext(to, vp, fn);
        }
    }
}

#endif //AR_CONTEXT_SWITCHER_HPP
