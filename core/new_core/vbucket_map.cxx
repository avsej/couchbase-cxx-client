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

#include "vbucket_map.hxx"
#include "core/utils/crc32.hxx"
#include "errors.hxx"

#include <fmt/core.h>
#include <fmt/format.h>

namespace couchbase::new_core
{
vbucket_map::vbucket_map(std::vector<std::vector<int>> entries, int num_replicas)
  : entries_{ std::move(entries) }
  , num_replicas_(num_replicas)
{
}

bool
vbucket_map::is_valid() const
{
    return !entries_.empty() && !entries_[0].empty();
}

std::size_t
vbucket_map::number_of_vbuckets() const
{
    return entries_.size();
}

int
vbucket_map::number_of_replicas() const
{
    return num_replicas_;
}

std::uint16_t
vbucket_map::vbucket_by_key(const std::string& key) const
{
    std::uint32_t crc = utils::hash_crc32(key.begin(), key.end());
    return static_cast<std::uint16_t>(crc % entries_.size());
}

std::pair<int, std::error_code>
vbucket_map::node_by_vbucket(std::uint16_t vbucket_id, std::uint32_t replica_id) const
{
    if (vbucket_id >= static_cast<std::uint16_t>(entries_.size())) {
        return { 0, core_errc::invalid_vbucket };
    }

    if (replica_id >= static_cast<std::uint32_t>(entries_.size())) {
        return { 0, core_errc::invalid_replica };
    }

    return { entries_[vbucket_id][replica_id], {} };
}

std::pair<std::vector<std::vector<std::uint16_t>>, std::error_code>
vbucket_map::vbuckets_by_server(int replica_id) const
{
    // We do not currently support listing for all replicas at once
    if (replica_id < 0) {
        return { {}, core_errc::invalid_replica };
    }

    auto replica_index = static_cast<std::size_t>(replica_id);

    std::vector<std::vector<std::uint16_t>> vb_list;

    for (std::size_t vbucket_id = 0; vbucket_id < entries_.size(); ++vbucket_id) {
        const auto& entry = entries_[vbucket_id];
        if (entry.size() <= replica_index) {
            continue;
        }

        auto server_id = static_cast<std::size_t>(entry[replica_index]);
        vb_list.resize(server_id + 1);
        vb_list[server_id].push_back(static_cast<std::uint16_t>(vbucket_id));
    }
    return { vb_list, {} };
}

std::pair<std::vector<std::uint16_t>, std::error_code>
vbucket_map::vbuckets_on_server(int index) const
{
    if (index < 0) {
        return { {}, core_errc::invalid_server };
    }
    auto [vb_list, ec] = vbuckets_by_server(0);
    if (ec) {
        return { {}, ec };
    }
    if (vb_list.size() <= static_cast<std::size_t>(index)) {
        return { {}, core_errc::invalid_replica };
    }
    return { vb_list[static_cast<std::size_t>(index)], {} };
}

std::pair<int, std::error_code>
vbucket_map::node_by_key(const std::string& key, std::uint32_t replica_id) const
{
    return node_by_vbucket(vbucket_by_key(key), replica_id);
}

std::string
vbucket_map::debug_string() const
{
    std::vector<char> out;
    fmt::format_to(std::back_insert_iterator(out), "vBucket map: [");
    auto sentinel = std::end(entries_);
    if (auto it = std::begin(entries_); it != sentinel) {
        fmt::format_to(std::back_insert_iterator(out), "[{}]", fmt::join(*it, ","));
        ++it;
        while (it != sentinel) {
            fmt::format_to(std::back_insert_iterator(out), ",[{}]", fmt::join(*it, ","));
            ++it;
        }
    }
    fmt::format_to(std::back_insert_iterator(out), "]");
    return { out.begin(), out.end() };
}

} // namespace couchbase::new_core
