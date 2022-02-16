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

#include "auth_provider.hxx"
#include "bucket_capabilities.hxx"
#include "route_config.hxx"
#include "tls_configuration.hxx"

#include <cstddef>
#include <memory>
#include <vector>

namespace couchbase::new_core
{
class mcbp_pipeline;

class mcbp_multiplexer_state
{
  public:
    mcbp_multiplexer_state(std::shared_ptr<route_config> config,
                           std::vector<std::shared_ptr<mcbp_pipeline>> pipelines,
                           std::shared_ptr<mcbp_pipeline> dead_pipeline,
                           std::vector<route_endpoint> server_list,
                           std::shared_ptr<auth_provider> auth,
                           std::shared_ptr<tls_configuration> tls_config);

    [[nodiscard]] auto type() const -> bucket_type;
    [[nodiscard]] auto config() const -> const route_config&;
    [[nodiscard]] auto vb_map() const -> std::shared_ptr<vbucket_map>;
    [[nodiscard]] auto ketama_map() const -> std::shared_ptr<ketama_continuum>;
    [[nodiscard]] auto rev_id() const -> std::int64_t;
    [[nodiscard]] auto number_of_pipelines() const -> std::size_t;
    [[nodiscard]] auto get_pipeline(std::size_t index) const -> std::shared_ptr<mcbp_pipeline>;
    [[nodiscard]] auto key_value_endpoints() const -> std::vector<std::string>;
    [[nodiscard]] auto has_bucket_capability(bucket_capability capability) const -> bool;
    [[nodiscard]] auto auth() const -> std::shared_ptr<auth_provider>;
    [[nodiscard]] auto tls_config() const -> std::shared_ptr<tls_configuration>;

    [[nodiscard]] auto debug_string() const -> std::string;

  private:
    const route_config route_config_;
    std::vector<std::shared_ptr<mcbp_pipeline>> pipelines_;
    std::shared_ptr<mcbp_pipeline> dead_pipeline_;
    std::vector<route_endpoint> server_list_;
    std::shared_ptr<auth_provider> auth_provider_;
    std::shared_ptr<tls_configuration> tls_config_;
    bucket_capabilities bucket_capabilities_{};
    bool collections_supported_{ false };
};
} // namespace couchbase::new_core
