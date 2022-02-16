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

#include "../bucket_type.hxx"
#include "../config_manager.hxx"
#include "../config_manager_properties.hxx"
#include "../config_value.hxx"
#include "../route_config.hxx"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace couchbase::new_core::impl
{
class config_management_component
{
  public:
    explicit config_management_component(const configuration_manager_properties& properties)
      : use_ssl_{ properties.use_tls }
      , no_tls_seed_node_{ properties.no_tls_seed_node }
      , network_type_{ properties.network_type }
      , current_config_(std::make_shared<route_config>())
    {
        source_servers_.reserve(properties.source_mcbp_addresses.size() + properties.source_http_addresses.size());
        std::copy(properties.source_mcbp_addresses.begin(), properties.source_mcbp_addresses.end(), source_servers_.end());
        std::copy(properties.source_http_addresses.begin(), properties.source_http_addresses.end(), source_servers_.end());
    }

    void use_tls(bool use)
    {
        use_ssl_ = use;
    }

    [[nodiscard]] const std::string& network_type() const
    {
        return network_type_;
    }

    void add_config_watcher(std::shared_ptr<route_config_watcher> watcher);

    void remove_config_watcher(std::shared_ptr<route_config_watcher> watcher);

    void on_new_config(const config_value& input);

  private:
    bool update_route_config(std::shared_ptr<route_config> config);

    std::shared_ptr<route_config> build_first_route_config(const config_value& config, bool use_ssl);

    std::atomic_bool use_ssl_{ false };
    bool no_tls_seed_node_;
    std::string network_type_{ "default" };
    std::vector<route_endpoint> source_servers_;

    bool seen_config_{ false };
    std::shared_ptr<route_config> current_config_{};

    std::vector<std::shared_ptr<route_config_watcher>> config_change_watchers_{};
    std::mutex watchers_mutex_{};
};
} // namespace couchbase::new_core::impl
