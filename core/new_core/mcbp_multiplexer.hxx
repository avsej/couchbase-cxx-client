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

#include "auth_handler.hxx"
#include "bucket_capabilities_verifier.hxx"
#include "config_management_component.hxx"
#include "error_map_component.hxx"
#include "mcbp_client_dialer_component.hxx"
#include "mcbp_multiplexer_properties.hxx"
#include "mcbp_multiplexer_state.hxx"
#include "tracer_component.hxx"

#include <memory>
#include <mutex>

namespace couchbase::new_core
{
class mcbp_multiplexer
  : public std::enable_shared_from_this<mcbp_multiplexer>
  , public route_config_watcher
  , public bucket_capabilities_verifier
{
  public:
    [[deprecated("use make_mcbp_multiplexer() instead of constructor")]] mcbp_multiplexer(
      const mcbp_multiplexer_properties& properties,
      config_management_component config_manager,
      error_map_component err_map,
      tracer_component tracer,
      mcbp_client_dialer_component dialer,
      std::shared_ptr<mcbp_multiplexer_state> mux_state);

    void on_new_route_config(std::shared_ptr<route_config> config) override;

    [[nodiscard]] auto has_bucket_capability(bucket_capability capability) const -> bool override;

  private:
    [[nodiscard]] auto new_multiplexer_state(std::shared_ptr<route_config> config,
                                             std::shared_ptr<tls_configuration> tls_config,
                                             std::shared_ptr<auth_provider> auth) -> std::shared_ptr<mcbp_multiplexer_state>;

    [[nodiscard]] auto build_auth_handler(std::shared_ptr<auth_provider> auth) -> auth_handler;

    bool collections_enabled_;
    bool no_tls_seed_node_;
    std::size_t queue_size_;
    std::size_t pool_size_;

    config_management_component config_manager_component_;
    error_map_component error_map_component_;
    tracer_component tracer_component_;
    mcbp_client_dialer_component mcbp_client_dialer_component_;

    std::mutex state_mutex_;
    std::shared_ptr<mcbp_multiplexer_state> state_;

    friend auto make_mcbp_multiplexer(const mcbp_multiplexer_properties&,
                                      config_management_component,
                                      error_map_component,
                                      tracer_component,
                                      mcbp_client_dialer_component,
                                      std::shared_ptr<mcbp_multiplexer_state>) -> std::shared_ptr<mcbp_multiplexer>;
};

[[nodiscard]] auto
make_mcbp_multiplexer(const mcbp_multiplexer_properties& properties,
                      config_management_component config_manager,
                      error_map_component err_map,
                      tracer_component tracer,
                      mcbp_client_dialer_component dialer,
                      std::shared_ptr<mcbp_multiplexer_state> mux_state) -> std::shared_ptr<mcbp_multiplexer>;
} // namespace couchbase::new_core
