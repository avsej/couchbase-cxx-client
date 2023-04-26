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

#include "configuration.hxx"

#include "configuration_fwd.hxx"
#include "configuration_json.hxx"
#include "core/utils/json.hxx"

#include <fmt/core.h>

#include <algorithm>
#include <stdexcept>

namespace couchbase::core::topology
{
bool
configuration::has_node_with_hostname(const std::string& hostname) const
{
    return std::any_of(nodes.begin(), nodes.end(), [&hostname](const auto& n) { return n.hostname == hostname; });
}

std::string
configuration::select_network(const std::string& bootstrap_hostname) const
{
    for (const auto& n : nodes) {
        if (n.this_node) {
            if (n.hostname == bootstrap_hostname) {
                return "default";
            }
            for (const auto& [network, address] : n.alt) {
                if (address.hostname == bootstrap_hostname) {
                    return network;
                }
            }
        }
    }
    return "default";
}

std::string
configuration::rev_str() const
{
    if (epoch) {
        return fmt::format("{}:{}", epoch.value(), rev.value_or(0));
    }
    return rev ? fmt::format("{}", *rev) : "(none)";
}

std::size_t
configuration::index_for_this_node() const
{
    for (const auto& n : nodes) {
        if (n.this_node) {
            return n.index;
        }
    }
    throw std::runtime_error("no nodes marked as this_node");
}

std::optional<std::size_t>
configuration::server_by_vbucket(std::uint16_t vbucket, std::size_t index)
{
    if (!vbmap.has_value() || vbucket >= vbmap->size()) {
        return {};
    }
    if (auto server_index = vbmap->at(vbucket)[index]; server_index >= 0) {
        return static_cast<std::size_t>(server_index);
    }
    return {};
}

std::pair<std::uint16_t, std::optional<std::size_t>>
configuration::map_key(std::string_view key, std::size_t index)
{
    if (!vbmap.has_value()) {
        return { 0, {} };
    }
    std::uint32_t crc = utils::hash_crc32(key.data(), key.size());
    auto vbucket = static_cast<std::uint16_t>(crc % vbmap->size());
    return { vbucket, server_by_vbucket(vbucket, index) };
}

configuration
make_blank_configuration(const std::string& hostname, std::uint16_t plain_port, std::uint16_t tls_port)
{
    configuration result;
    result.id = couchbase::core::uuid::random();
    result.epoch = 0;
    result.rev = 0;
    result.nodes.resize(1);
    result.nodes[0].hostname = hostname;
    result.nodes[0].this_node = true;
    result.nodes[0].services_plain.key_value = plain_port;
    result.nodes[0].services_tls.key_value = tls_port;
    return result;
}

configuration
make_blank_configuration(const std::vector<std::pair<std::string, std::string>>& endpoints, bool use_tls, bool force)
{
    configuration result;
    result.force = force;
    result.id = couchbase::core::uuid::random();
    result.epoch = 0;
    result.rev = 0;
    result.nodes.resize(endpoints.size());
    std::size_t idx{ 0 };
    for (const auto& [hostname, port] : endpoints) {
        topology::node node{ false, idx++, hostname };
        if (use_tls) {
            node.services_tls.key_value = std::stol(port);
        } else {
            node.services_plain.key_value = std::stol(port);
        }
        result.nodes.emplace_back(node);
    }
    return result;
}

auto
configuration_get_number_of_replicas(const configuration& config) -> std::uint32_t
{
    return config.num_replicas.value_or(0U);
}

auto
configuration_diff_nodes(const configuration& lhs, const configuration& rhs) -> std::vector<topology::node>
{
    std::vector<topology::node> output;
    for (const auto& re : rhs.nodes) {
        bool known = false;
        for (const auto& le : lhs.nodes) {
            if (le.hostname == re.hostname && le.services_plain.management.value_or(0) == re.services_plain.management.value_or(0)) {
                known = true;
                break;
            }
        }
        if (!known) {
            output.push_back(re);
        }
    }
    return output;
}

auto
parse_configuration(std::string_view input, std::string_view endpoint_address, std::uint16_t endpoint_port) -> configuration
{
    auto config = utils::json::parse(input).as<topology::configuration>();
    for (auto& node : config.nodes) {
        if (node.hostname == "$HOST") {
            node.hostname = endpoint_address;
        }
    }

    // workaround for servers which don't specify this_node
    {
        bool has_this_node = false;
        for (const auto& node : config.nodes) {
            if (node.this_node) {
                has_this_node = true;
                break;
            }
        }

        if (!has_this_node) {
            for (auto& node : config.nodes) {
                auto kv_port = node.port_or(couchbase::core::service_type::key_value, false, 0);
                auto kv_tls_port = node.port_or(couchbase::core::service_type::key_value, true, 0);
                if (node.hostname == endpoint_address && (kv_port == endpoint_port || kv_tls_port == endpoint_port)) {
                    node.this_node = true;
                    break;
                }
            }
        }
    }

    return config;
}

} // namespace couchbase::core::topology
