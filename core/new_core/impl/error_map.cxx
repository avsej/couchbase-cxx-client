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

#include "error_map.hxx"
#include "core/utils/json.hxx"

#include "couchbase/error_codes.hxx"

#include <gsl/gsl_narrow>

namespace couchbase::new_core::impl
{
auto
parse_key_value_error_map(const std::vector<std::byte>& input) -> std::pair<std::shared_ptr<error_map>, std::error_code>
{
    if (input.empty()) {
        return { {}, errc::common::invalid_argument };
    }

    try {
        auto result = std::make_shared<error_map>();
        if (auto json = core::utils::json::parse_binary(input); json.is_object()) {
            result->version = json.at("version").as<std::uint32_t>();
            result->revision = json.at("revision").as<std::uint32_t>();

            for (const auto& [error, definition] : json.at("errors").get_object()) {
                auto code = gsl::narrow_cast<std::uint16_t>(std::stoul(error, nullptr, 16));

                error_map_error entry;

                const auto& info = definition.get_object();
                entry.name = info.at("name").get_string();
                entry.description = info.at("desc").get_string();
                for (const auto& attribute : info.at("attrs").get_array()) {
                    entry.attributes.insert(attribute.get_string());
                }

                if (const auto* retry_json = definition.find("retry"); retry_json != nullptr && retry_json->is_object()) {
                    error_map_retry retry;
                    retry.strategy = retry_json->optional<std::string>("strategy").value_or("");
                    retry.interval = std::chrono::milliseconds{ retry_json->optional<std::uint64_t>("interval").value_or(0) };
                    retry.after = std::chrono::milliseconds{ retry_json->optional<std::uint64_t>("after").value_or(0) };
                    retry.ceil = std::chrono::milliseconds{ retry_json->optional<std::uint64_t>("ceil").value_or(0) };
                    retry.max_duration = std::chrono::milliseconds{ retry_json->optional<std::uint64_t>("max-duration").value_or(0) };
                    entry.retry = retry;
                }

                result->errors.try_emplace(code, entry);
            }
        }
        return { result, {} };
    } catch (const tao::pegtl::parse_error&) {
        return { {}, errc::common::parsing_failure };
    }
}
} // namespace couchbase::new_core::impl
