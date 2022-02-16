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

#include "mcbp_multiplexer.hxx"
#include "core/logger/logger.hxx"

namespace couchbase::new_core
{

mcbp_multiplexer::mcbp_multiplexer(const mcbp_multiplexer_properties& properties,
                                   config_management_component config_manager,
                                   error_map_component err_map,
                                   tracer_component tracer,
                                   mcbp_client_dialer_component dialer,
                                   std::shared_ptr<mcbp_multiplexer_state> mux_state)
  : collections_enabled_{ properties.collections_enabled }
  , no_tls_seed_node_{ properties.no_tls_seed_node }
  , queue_size_{ properties.queue_size }
  , pool_size_{ properties.pool_size }
  , config_manager_component_{ std::move(config_manager) }
  , error_map_component_{ std::move(err_map) }
  , tracer_component_{ std::move(tracer) }
  , mcbp_client_dialer_component_{ std::move(dialer) }
  , state_{ std::move(mux_state) }
{
}

static auto
new_server_list_debug_string(const std::string& bucket_name, const std::vector<route_endpoint>& server_list) -> std::string
{
    std::vector<char> out;

    fmt::format_to(std::back_insert_iterator(out), "mcbp_multiplexer applying endpoints. bucket: {}", bucket_name);
    for (const auto& item : server_list) {
        fmt::format_to(std::back_insert_iterator(out), "\n  - {}, seed: {}", item.address, item.is_seed_node);
    }

    return { out.begin(), out.end() };
}

auto
mcbp_multiplexer::new_multiplexer_state(std::shared_ptr<route_config> config,
                                        std::shared_ptr<tls_configuration> tls_config,
                                        std::shared_ptr<auth_provider> auth) -> std::shared_ptr<mcbp_multiplexer_state>
{
    std::size_t pool_size = 1;
    if (config->is_gcccp_config()) {
        pool_size = pool_size_;
    }

    bool use_tls = tls_config != nullptr;

    std::vector<route_endpoint> server_list;
    if (use_tls) {
        server_list = config->key_value_endpoints().ssl_endpoints;
        if (no_tls_seed_node_) {
            const auto& non_ssl_endpoints = config->key_value_endpoints().non_ssl_endpoints;
            for (std::size_t i = 0; i < non_ssl_endpoints.size(); ++i) {
                if (non_ssl_endpoints[i].is_seed_node) {
                    server_list[i] = non_ssl_endpoints[i];
                    break;
                }
            }
        }
    } else {
        server_list = config->key_value_endpoints().non_ssl_endpoints;
    }

    LOG_DEBUG_RAW(new_server_list_debug_string(config->name(), server_list));

    auto handler = build_auth_handler(auth);

    return nullptr;
}

auto
mcbp_multiplexer::build_auth_handler(std::shared_ptr<auth_provider> auth) -> auth_handler
{
    return [](std::shared_ptr<auth_client> client, std::chrono::steady_clock::time_point deadline, auth_mechanism mechanism) {

    };
}

void
mcbp_multiplexer::on_new_route_config(std::shared_ptr<route_config> config)
{
    std::scoped_lock lock(state_mutex_);

    auto old_state = state_;
    auto new_state = new_multiplexer_state(std::move(config), old_state->tls_config(), old_state->auth());
    if (!std::atomic_compare_exchange_strong(&state_, &old_state, new_state)) {
        LOG_WARNING("someone preempted the config update, ignoring");
        return;
    }

    if (old_state == nullptr) {
    }
}

auto
make_mcbp_multiplexer(const mcbp_multiplexer_properties& properties,
                      config_management_component config_manager,
                      error_map_component err_map,
                      tracer_component tracer,
                      mcbp_client_dialer_component dialer,
                      std::shared_ptr<mcbp_multiplexer_state> mux_state) -> std::shared_ptr<mcbp_multiplexer>
{
    auto mux = std::make_shared<mcbp_multiplexer>(
      properties, std::move(config_manager), std::move(err_map), std::move(tracer), std::move(dialer), std::move(mux_state));
    mux->config_manager_component_.add_config_watcher(mux);
    return mux;
}

bool
mcbp_multiplexer::has_bucket_capability(bucket_capability capability) const
{
    auto state = state_;
    if (state != nullptr) {
        return state->has_bucket_capability(capability);
    }
    return false;
}

} // namespace couchbase::new_core
