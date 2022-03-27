//
// Created by df on 12/29/21.
//

#ifndef AR_BASE64_H
#define AR_BASE64_H

#include <iostream>

namespace AsyncRuntime {



    /**
     * @brief
     * @param val
     * @return
     */
    std::string Base64Decode(const std::string &val);


    /**
     * @brief
     * @param val
     * @return
     */
    std::string Base64Encode(const std::string &val);
}

#endif //AR_BASE64_H
