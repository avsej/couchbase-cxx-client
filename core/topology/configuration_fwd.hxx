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

#include <cinttypes>
#include <string>
#include <string_view>
#include <vector>

namespace couchbase::core::topology
{
struct configuration;
struct node;

auto
configuration_diff_nodes(const configuration& lhs, const configuration& rhs) -> std::vector<topology::node>;

auto
configuration_get_number_of_replicas(const configuration& config) -> std::uint32_t;

auto
parse_configuration(std::string_view input, std::string_view endpoint_address, std::uint16_t endpoint_port) -> configuration;

auto
make_blank_configuration(const std::string& hostname, std::uint16_t plain_port, std::uint16_t tls_port) -> configuration;

auto
make_blank_configuration(const std::vector<std::pair<std::string, std::string>>& endpoints, bool use_tls, bool force = false)
  -> configuration;
} // namespace couchbase::core::topology
