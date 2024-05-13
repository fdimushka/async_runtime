#ifndef TEYE_STREAMSERVER_HTTP_DIGEST_AUTHENTICATOR_H
#define TEYE_STREAMSERVER_HTTP_DIGEST_AUTHENTICATOR_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "ar/task.hpp"

namespace AsyncRuntime::IO {

    class http_digest_authenticator {
    public:
        http_digest_authenticator(std::string www_authenticate,
                                  std::string username, std::string password,
                                  std::string uri,
                                  std::string method) noexcept;

        bool generate_authorization();

        [[nodiscard]] std::string get_authorization() const noexcept { return m_authorization; }
        [[nodiscard]] std::string get_response() const noexcept { return m_response; }
    private:
        bool find_nonce() { return find_section("nonce", m_nonce); }
        bool find_realm() { return find_section("realm", m_realm); }

        virtual std::string generate_nonce();

        bool find_section(const std::string& key, std::string& value) const;

        std::string calculate_response() noexcept;

        std::string m_authenticate;
        std::string m_username;
        std::string m_password;
        std::string m_realm;
        std::string m_nonce;
        std::string m_algorithm;
        std::string m_uri;
        std::string m_method;

        std::string m_cnonce;
        std::string m_nonceCount;
        std::string m_ha1;
        std::string m_ha2;
        std::string m_response;
        std::string m_authorization;
    };
}

#endif //TEYE_STREAMSERVER_HTTP_DIGEST_AUTHENTICATOR_H
