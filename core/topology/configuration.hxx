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

#pragma once

#include "configuration_fwd.hxx"

#include "capabilities.hxx"
#include "core/platform/uuid.h"
#include "core/utils/crc32.hxx"
#include "node.hxx"

#include <optional>
#include <set>

namespace couchbase::core::topology
{
enum class node_locator_type {
    unknown,
    vbucket,
    ketama,
};

struct configuration {
    [[nodiscard]] std::string select_network(const std::string& bootstrap_hostname) const;

    using vbucket_map = typename std::vector<std::vector<std::int16_t>>;

    std::optional<std::int64_t> epoch{};
    std::optional<std::int64_t> rev{};
    couchbase::core::uuid::uuid_t id{};
    std::optional<std::uint32_t> num_replicas{};
    std::vector<node> nodes{};
    std::optional<std::string> uuid{};
    std::optional<std::string> bucket{};
    std::optional<vbucket_map> vbmap{};
    std::optional<std::uint64_t> collections_manifest_uid{};
    std::set<bucket_capability> bucket_capabilities{};
    std::set<cluster_capability> cluster_capabilities{};
    node_locator_type node_locator{ node_locator_type::unknown };
    bool force{ false };

    bool operator==(const configuration& other) const
    {
        return epoch == other.epoch && rev == other.rev;
    }

    bool operator<(const configuration& other) const
    {
        return epoch < other.epoch || (epoch == other.epoch && rev < other.rev);
    }

    bool operator>(const configuration& other) const
    {
        return other < *this;
    }

    [[nodiscard]] std::string rev_str() const;

    [[nodiscard]] bool supports_enhanced_prepared_statements() const
    {
        return cluster_capabilities.find(cluster_capability::n1ql_enhanced_prepared_statements) != cluster_capabilities.end();
    }

    [[nodiscard]] bool ephemeral() const
    {
        // Use bucket capabilities to identify if couchapi is missing (then its ephemeral). If its null then
        // we are running an old version of couchbase which doesn't have ephemeral buckets at all.
        return bucket_capabilities.count(couchbase::core::bucket_capability::couchapi) == 0;
    }

    [[nodiscard]] std::size_t index_for_this_node() const;
    [[nodiscard]] bool has_node_with_hostname(const std::string& hostname) const;

    std::pair<std::uint16_t, std::optional<std::size_t>> map_key(std::string_view key, std::size_t index);

    std::optional<std::size_t> server_by_vbucket(std::uint16_t vbucket, std::size_t index);
};
} // namespace couchbase::core::topology
