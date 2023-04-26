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

#include "core/service_type.hxx"

#include <cinttypes>
#include <map>
#include <optional>
#include <string>

namespace couchbase::core::topology
{
struct port_map {
    std::optional<std::uint16_t> key_value{};
    std::optional<std::uint16_t> management{};
    std::optional<std::uint16_t> analytics{};
    std::optional<std::uint16_t> search{};
    std::optional<std::uint16_t> views{};
    std::optional<std::uint16_t> query{};
    std::optional<std::uint16_t> eventing{};
};

struct alternate_address {
    std::string name{};
    std::string hostname{};
    port_map services_plain{};
    port_map services_tls{};
};

struct node {
    bool this_node{ false };
    size_t index{};
    std::string hostname{};
    port_map services_plain{};
    port_map services_tls{};
    std::map<std::string, alternate_address> alt{};

    [[nodiscard]] std::uint16_t port_or(service_type type, bool is_tls, std::uint16_t default_value) const;

    [[nodiscard]] std::uint16_t port_or(const std::string& network, service_type type, bool is_tls, std::uint16_t default_value) const;

    [[nodiscard]] const std::string& hostname_for(const std::string& network) const;

    [[nodiscard]] std::optional<std::string> endpoint(const std::string& network, service_type type, bool is_tls) const;
};
} // namespace couchbase::core::topology
