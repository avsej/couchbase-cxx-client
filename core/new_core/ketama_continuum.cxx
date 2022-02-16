/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2022-Present Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "ketama_continuum.hxx"
#include "errors.hxx"

#include <fmt/core.h>
#include <openssl/evp.h>

#include <algorithm>
#include <array>

namespace couchbase::new_core
{

template<typename T>
static std::vector<std::byte>
digest_md5(const T& data)
{
    std::vector<std::byte> ret{};
    ret.resize(EVP_MAX_MD_SIZE);

    EVP_MD_CTX* md = EVP_MD_CTX_new();
    if (md == nullptr) {
        throw std::system_error(core_errc::client_internal_error, "digest_md5: failed to create EVP_MD_CTX");
    }
    if (EVP_DigestInit_ex(md, EVP_md5(), nullptr) != 0) {
        throw std::system_error(core_errc::client_internal_error, "digest_md5: failed to initialize EVP_MD_CTX with EVP_md5");
    }
    if (EVP_DigestUpdate(md, data.data(), data.size()) != 0) {
        throw std::system_error(core_errc::client_internal_error, "digest_md5: failed to update EVP_MD_CTX with new data");
    }
    unsigned int bytes_written{ 0 };
    if (EVP_DigestFinal(md, reinterpret_cast<unsigned char*>(ret.data()), &bytes_written) != 0) {
        throw std::system_error(core_errc::client_internal_error, "digest_md5: failed to update EVP_MD_CTX with new data");
    }
    if (static constexpr int md5_digest_size{ 16 }; bytes_written != md5_digest_size) {
        throw std::system_error(core_errc::client_internal_error,
                                fmt::format("digest_md5: unexpected size of the digest: {} (expected {})", bytes_written, md5_digest_size));
    }
    ret.resize(bytes_written);
    return ret;
}

ketama_continuum::ketama_continuum(const std::vector<route_endpoint>& endpoint_list)
{
    std::vector<std::string> server_list{};
    server_list.reserve(endpoint_list.size());
    for (const auto& s : endpoint_list) {
        server_list.push_back(s.address);
    }

    // libcouchbase presorts this. Might not strictly be required
    std::sort(server_list.begin(), server_list.end(), std::less<>());

    for (std::size_t ss = 0; ss < server_list.size(); ++ss) {
        // 160 points per server
        for (std::size_t hh = 0; hh < 40; ++hh) {
            auto hostkey = fmt::format("{}-{}", server_list[ss], hh);
            auto digest = digest_md5(hostkey);

            for (std::size_t nn = 0; nn < 4; ++nn) {
                auto d1 = static_cast<std::uint32_t>(digest[3 + nn * 4] & static_cast<std::byte>(0xffU)) << 24U;
                auto d2 = static_cast<std::uint32_t>(digest[2 + nn * 4] & static_cast<std::byte>(0xffU)) << 16U;
                auto d3 = static_cast<std::uint32_t>(digest[1 + nn * 4] & static_cast<std::byte>(0xffU)) << 8U;
                auto d4 = static_cast<std::uint32_t>(digest[0 + nn * 4] & static_cast<std::byte>(0xffU));
                auto point = d1 | d2 | d3 | d4;
                entries_.emplace_back(route_ketama_continuum{ point, static_cast<std::uint32_t>(ss) });
            }
        }
    }

    std::sort(entries_.begin(), entries_.end(), [](const auto& lhs, const auto& rhs) { return lhs.point < rhs.point; });
}

bool
ketama_continuum::is_valid() const
{
    return !entries_.empty();
}

std::pair<int, std::error_code>
ketama_continuum::node_by_hash(std::uint32_t hash) const
{
    if (entries_.empty()) {
        return { 0, core_errc::client_internal_error };
    }

    // copied from libcouchbase vbucket.c (map_ketama)
    std::uint32_t lowp{ 0 };
    std::uint32_t highp{ static_cast<uint32_t>(entries_.size()) };
    auto maxp = highp;
    while (true) {
        auto midp = lowp + (highp - lowp) / 2;
        if (midp == maxp) {
            return { static_cast<int>(entries_[0].index), {} };
        }

        auto mid = entries_[midp].point;
        if (auto prev = (midp == 0) ? 0 : entries_[midp - 1].point; hash <= mid && hash > prev) {
            return { static_cast<int>(entries_[midp].index), {} };
        }

        if (mid < hash) {
            lowp = midp + 1;
        } else {
            highp = midp - 1;
        }

        if (lowp > highp) {
            return { static_cast<int>(entries_[0].index), {} };
        }
    }
}

std::uint32_t
ketama_hash(const std::vector<std::byte>& key)
{
    auto digest = digest_md5(key);

    return ((static_cast<std::uint32_t>(digest[3]) & 0xffU) << 24U | (static_cast<std::uint32_t>(digest[2]) & 0xffU) << 16U |
            (static_cast<std::uint32_t>(digest[1]) & 0xffU) << 8U | (static_cast<std::uint32_t>(digest[0]) & 0xffU)) &
           0xffffffffU;
}

std::pair<int, std::error_code>
ketama_continuum::node_by_key(const std::vector<std::byte>& key) const
{
    return node_by_hash(ketama_hash(key));
}

std::string
ketama_continuum::debug_string() const
{
    std::vector<char> out;
    fmt::format_to(std::back_insert_iterator(out), "ketama map: [");
    auto sentinel = std::end(entries_);
    if (auto it = std::begin(entries_); it != sentinel) {
        fmt::format_to(std::back_insert_iterator(out), "[{},{}]", it->index, it->point);
        ++it;
        while (it != sentinel) {
            fmt::format_to(std::back_insert_iterator(out), ",[{},{}]", it->index, it->point);
            ++it;
        }
    }
    fmt::format_to(std::back_insert_iterator(out), "]");
    return { out.begin(), out.end() };
}
} // namespace couchbase::new_core
