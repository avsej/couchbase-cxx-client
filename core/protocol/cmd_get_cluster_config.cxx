/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2020-2021 Couchbase, Inc.
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

#include "cmd_get_cluster_config.hxx"

#include "core/logger/logger.hxx"
#include "core/topology/configuration_json.hxx"
#include "core/utils/json.hxx"

#include <gsl/assert>

namespace couchbase::core::protocol
{
bool
get_cluster_config_response_body::parse(key_value_status_code status,
                                        const header_buffer& header,
                                        std::uint8_t framing_extras_size,
                                        std::uint16_t key_size,
                                        std::uint8_t extras_size,
                                        const std::vector<std::byte>& body,
                                        const cmd_info& info)
{
    Expects(header[1] == static_cast<std::byte>(opcode));
    if (status == key_value_status_code::success) {
        std::vector<std::uint8_t>::difference_type offset = framing_extras_size + key_size + extras_size;
        std::string_view config_text{ reinterpret_cast<const char*>(body.data()) + offset, body.size() - static_cast<std::size_t>(offset) };
        try {
            config_ = std::make_unique<topology::configuration>(
              topology::parse_configuration(config_text, info.endpoint_address, info.endpoint_port));
        } catch (const tao::pegtl::parse_error& e) {
            CB_LOG_DEBUG("unable to parse cluster configuration as JSON: {}, {}", e.message(), config_text);
        }
        return true;
    }
    return false;
}
topology::configuration
get_cluster_config_response_body::config() const
{
    if (config_) {
        return *config_;
    }
    return {};
}
} // namespace couchbase::core::protocol
