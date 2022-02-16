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

#include "config_utils.hxx"

#include "../errors.hxx"

#include "core/logger/logger.hxx"
#include "core/utils/json.hxx"

#include <couchbase/error_codes.hxx>

namespace couchbase::new_core::utils
{

auto
parse_config(std::vector<std::byte>& input, std::string source_host) -> std::pair<config_value, std::error_code>
{
    replace_host_placeholder(input, source_host);
    try {
        return { { core::utils::json::parse_binary(input), std::move(source_host) }, {} };
    } catch (const tao::pegtl::parse_error& e) {
        LOG_WARNING("unable to parse configuration as JSON: {}", e.message());
        return { {}, core_errc::parsing_failure };
    }
}

void
replace_host_placeholder(std::vector<std::byte>& data, const std::string& source_host)
{
    if (source_host.find("$HOST") != std::string::npos) {
        throw std::system_error(errc::common::invalid_argument, "replace_host_placeholder: replacement cannot contain $HOST");
    }

    static std::array host_placeholder{
        std::byte{ 0x24 }, // '$'
        std::byte{ 0x48 }, // 'H'
        std::byte{ 0x4f }, // 'O'
        std::byte{ 0x53 }, // 'S'
        std::byte{ 0x54 }, // 'T'
    };
    static_assert(sizeof(std::string::value_type) == sizeof(std::byte));
    std::vector<std::byte> replacement(source_host.size());
    for (std::size_t i = 0; i < source_host.size(); ++i) {
        replacement[i] = static_cast<std::byte>(source_host[i]);
    }

    while (true) {
        auto it = std::search(data.begin(), data.end(), host_placeholder.begin(), host_placeholder.end());
        if (it == data.end()) {
            return;
        }
        it = data.erase(it, it + host_placeholder.size());
        data.insert(it, replacement.begin(), replacement.end());
    }
}
} // namespace couchbase::new_core::utils
