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

#pragma once

#include <cinttypes>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

namespace couchbase::new_core
{
class vbucket_map
{
  public:
    vbucket_map(std::vector<std::vector<int>> entries, int num_replicas);

    [[nodiscard]] bool is_valid() const;

    [[nodiscard]] std::size_t number_of_vbuckets() const;

    [[nodiscard]] int number_of_replicas() const;

    [[nodiscard]] std::uint16_t vbucket_by_key(const std::string& key) const;

    [[nodiscard]] std::pair<int, std::error_code> node_by_vbucket(std::uint16_t vbucket_id, std::uint32_t replica_id) const;

    [[nodiscard]] std::pair<std::vector<std::vector<std::uint16_t>>, std::error_code> vbuckets_by_server(int replica_id) const;

    [[nodiscard]] std::pair<std::vector<std::uint16_t>, std::error_code> vbuckets_on_server(int index) const;

    [[nodiscard]] std::pair<int, std::error_code> node_by_key(const std::string& key, std::uint32_t replica_id) const;

    [[nodiscard]] std::string debug_string() const;

  private:
    std::vector<std::vector<int>> entries_;
    int num_replicas_;
};
} // namespace couchbase::new_core
