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

#include "node.hxx"

#include "core/logger/logger.hxx"
#include "core/service_type_fmt.hxx"

namespace couchbase::core::topology
{
std::uint16_t
node::port_or(service_type type, bool is_tls, std::uint16_t default_value) const
{
    if (is_tls) {
        switch (type) {
            case service_type::query:
                return services_tls.query.value_or(default_value);

            case service_type::analytics:
                return services_tls.analytics.value_or(default_value);

            case service_type::search:
                return services_tls.search.value_or(default_value);

            case service_type::view:
                return services_tls.views.value_or(default_value);

            case service_type::management:
                return services_tls.management.value_or(default_value);

            case service_type::key_value:
                return services_tls.key_value.value_or(default_value);

            case service_type::eventing:
                return services_tls.eventing.value_or(default_value);
        }
    }
    switch (type) {
        case service_type::query:
            return services_plain.query.value_or(default_value);

        case service_type::analytics:
            return services_plain.analytics.value_or(default_value);

        case service_type::search:
            return services_plain.search.value_or(default_value);

        case service_type::view:
            return services_plain.views.value_or(default_value);

        case service_type::management:
            return services_plain.management.value_or(default_value);

        case service_type::key_value:
            return services_plain.key_value.value_or(default_value);

        case service_type::eventing:
            return services_plain.eventing.value_or(default_value);
    }
    return default_value;
}

const std::string&
node::hostname_for(const std::string& network) const
{
    if (network == "default") {
        return hostname;
    }
    const auto& address = alt.find(network);
    if (address == alt.end()) {
        CB_LOG_WARNING(R"(requested network "{}" is not found, fallback to "default" host)", network);
        return hostname;
    }
    return address->second.hostname;
}

std::uint16_t
node::port_or(const std::string& network, service_type type, bool is_tls, std::uint16_t default_value) const
{
    if (network == "default") {
        return port_or(type, is_tls, default_value);
    }
    const auto& address = alt.find(network);
    if (address == alt.end()) {
        CB_LOG_WARNING(R"(requested network "{}" is not found, fallback to "default" port of {} service)", network, type);
        return port_or(type, is_tls, default_value);
    }
    if (is_tls) {
        switch (type) {
            case service_type::query:
                return address->second.services_tls.query.value_or(default_value);

            case service_type::analytics:
                return address->second.services_tls.analytics.value_or(default_value);

            case service_type::search:
                return address->second.services_tls.search.value_or(default_value);

            case service_type::view:
                return address->second.services_tls.views.value_or(default_value);

            case service_type::management:
                return address->second.services_tls.management.value_or(default_value);

            case service_type::key_value:
                return address->second.services_tls.key_value.value_or(default_value);

            case service_type::eventing:
                return address->second.services_tls.eventing.value_or(default_value);
        }
    }
    switch (type) {
        case service_type::query:
            return address->second.services_plain.query.value_or(default_value);

        case service_type::analytics:
            return address->second.services_plain.analytics.value_or(default_value);

        case service_type::search:
            return address->second.services_plain.search.value_or(default_value);

        case service_type::view:
            return address->second.services_plain.views.value_or(default_value);

        case service_type::management:
            return address->second.services_plain.management.value_or(default_value);

        case service_type::key_value:
            return address->second.services_plain.key_value.value_or(default_value);

        case service_type::eventing:
            return address->second.services_plain.eventing.value_or(default_value);
    }
    return default_value;
}

std::optional<std::string>
node::endpoint(const std::string& network, service_type type, bool is_tls) const
{
    auto p = port_or(type, is_tls, 0);
    if (p == 0) {
        return {};
    }
    return fmt::format("{}:{}", hostname_for(network), p);
}
} // namespace couchbase::core::topology
