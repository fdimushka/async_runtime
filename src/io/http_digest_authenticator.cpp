#include "ar/io/http_digest_authenticator.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/regex.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <random>

using namespace AsyncRuntime;
using namespace AsyncRuntime::IO;
using boost::uuids::detail::md5;

std::string to_string(const md5::digest_type& digest) {
    const auto charDigest = reinterpret_cast<const int*>(&digest);
    std::string result;
    boost::algorithm::hex(charDigest, charDigest + (sizeof(md5::digest_type) / sizeof(int)), std::back_inserter(result));
    for (auto& symbol : result)
        symbol = std::tolower(symbol);
    return result;
}

std::string calculateHA1(const std::string& m_username, const std::string& m_realm,
                         const std::string& m_password) noexcept {
    md5 ha1;
    md5::digest_type digest;
    std::string ha1_str = m_username + ':' + m_realm + ':' + m_password;
    ha1.process_bytes(ha1_str.data(), ha1_str.size());
    ha1.get_digest(digest);
    return to_string(digest);
}

std::string calculateHA2(const std::string& m_method, const std::string& m_uri) noexcept {
    md5 ha2;
    md5::digest_type digest;
    std::string ha2_str = m_method + ':' + m_uri;
    ha2.process_bytes(ha2_str.data(), ha2_str.size());
    ha2.get_digest(digest);
    return to_string(digest);
}
http_digest_authenticator::http_digest_authenticator(std::string www_authenticate, std::string username, std::string password,
                                         std::string uri, std::string method) noexcept
        : m_authenticate{std::move(www_authenticate)},
          m_username{std::move(username)},
          m_password{std::move(password)},
          m_uri{std::move(uri)},
          m_method{std::move(method)} {}

bool http_digest_authenticator::generate_authorization() {
    if (!find_nonce() || !find_realm())
        return false;

    m_cnonce = find_nonce();
    m_nonceCount = "1";
    m_ha1 = calculateHA1(m_username, m_realm, m_password);
    m_ha2 = calculateHA2(m_method, m_uri);
    m_response = calculate_response();

    m_authorization = "Digest username=\"" + m_username + "\", realm=\"" + m_realm + "\", nonce=\"" + m_nonce +
                      "\", uri=\"" + m_uri + "\", qop=auth, nc=" + m_nonceCount + ", cnonce=\"" + m_cnonce +
                      "\", response=\"" + m_response + "\"";
    return true;
}

std::string http_digest_authenticator::generate_nonce() {
    std::random_device rd;
    std::uniform_int_distribution<unsigned short> length{8, 32};
    std::uniform_int_distribution<unsigned short> distNum{0, 15};
    std::string nonce;
    nonce.resize(length(rd));
    constexpr char hex[16]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    for (char& val : nonce) {
        val = hex[distNum(rd)];
    }
    return nonce;
}

bool http_digest_authenticator::find_section(const std::string& key, std::string& value) const {
    boost::regex reg{key + "=([^,]+)"};
    auto start = m_authenticate.cbegin();
    auto end = m_authenticate.cend();
    boost::match_results<std::string::const_iterator> matches;
    boost::match_flag_type flags = boost::match_default;
    if (boost::regex_search(start, end, matches, reg, flags)) {
        auto size = static_cast<size_t>(std::distance(matches[1].first, matches[1].second));
        start = matches[1].first;
        end = matches[1].second - 1;
        if (*start == '"') {
            ++start;
            --size;
        }
        if (*end == '"') {
            --size;
        }
        value = std::string(start, start + size);
        return true;
    }
    return false;
}

std::string http_digest_authenticator::calculate_response() noexcept {
    md5 result;
    md5::digest_type digest;
    std::string result_str = m_ha1 + ':' + m_nonce + ':' + m_nonceCount + ':' + m_cnonce + ":auth:" + m_ha2;
    result.process_bytes(result_str.data(), result_str.size());
    result.get_digest(digest);
    return to_string(digest);
}
