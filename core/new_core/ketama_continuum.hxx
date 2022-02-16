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

#include "route_endpoint.hxx"

#include <cinttypes>
#include <system_error>
#include <utility>
#include <vector>

namespace couchbase::new_core
{
class ketama_continuum
{
  public:
    explicit ketama_continuum(const std::vector<route_endpoint>& endpoint_list);

    [[nodiscard]] bool is_valid() const;

    [[nodiscard]] std::pair<int, std::error_code> node_by_key(const std::vector<std::byte>& key) const;

    [[nodiscard]] std::string debug_string() const;

  private:
    [[nodiscard]] std::pair<int, std::error_code> node_by_hash(std::uint32_t hash) const;

    struct route_ketama_continuum {
        std::uint32_t index;
        std::uint32_t point;
    };

    std::vector<route_ketama_continuum> entries_;
};
} // namespace couchbase::new_core
