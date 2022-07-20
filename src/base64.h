#ifndef AR_BASE64_H
#define AR_BASE64_H

#include <string>

namespace AsyncRuntime::Base64 {
        /**
         * @brief base 64 encode
         * @param s
         * @param url
         * @return
         */
        std::string base64_encode     (std::string const& s, bool url = false);
        std::string base64_encode_pem (std::string const& s);
        std::string base64_encode_mime(std::string const& s);
        std::string base64_encode(unsigned char const*, size_t len, bool url = false);


        /**
         * @brief base64 decode
         * @param s
         * @param remove_linebreaks
         * @return
         */
        std::string base64_decode(std::string const& s, bool remove_linebreaks = false);
    }
#endif //AR_BASE64_H
