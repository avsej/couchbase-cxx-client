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

#include "mcbp_multiplexer_state.hxx"

#include "mcbp_pipeline.hxx"

#include <fmt/core.h>

namespace couchbase::new_core
{
mcbp_multiplexer_state::mcbp_multiplexer_state(std::shared_ptr<route_config> config,
                                               std::vector<std::shared_ptr<mcbp_pipeline>> pipelines,
                                               std::shared_ptr<mcbp_pipeline> dead_pipeline,
                                               std::vector<route_endpoint> server_list,
                                               std::shared_ptr<auth_provider> auth,
                                               std::shared_ptr<tls_configuration> tls_config)
  : route_config_{ *config }
  , pipelines_{ std::move(pipelines) }
  , dead_pipeline_{ std::move(dead_pipeline) }
  , server_list_{ std::move(server_list) }
  , auth_provider_{ std::move(auth) }
  , tls_config_{ std::move(tls_config) }
{
    if (route_config_.rev_id() >= 0) {
        bucket_capabilities_ = route_config_.bucket_caps();
        collections_supported_ = bucket_capabilities_.supports(bucket_capability::collections);
    }
}

auto
mcbp_multiplexer_state::number_of_pipelines() const -> std::size_t
{
    return pipelines_.size();
}

auto
mcbp_multiplexer_state::config() const -> const route_config&
{
    return route_config_;
}

auto
mcbp_multiplexer_state::rev_id() const -> std::int64_t
{
    return route_config_.rev_id();
}

auto
mcbp_multiplexer_state::type() const -> bucket_type
{
    return route_config_.type();
}

auto
mcbp_multiplexer_state::vb_map() const -> std::shared_ptr<vbucket_map>
{
    return route_config_.vb_map();
}

auto
mcbp_multiplexer_state::ketama_map() const -> std::shared_ptr<ketama_continuum>
{
    return route_config_.ketama_map();
}

auto
mcbp_multiplexer_state::key_value_endpoints() const -> std::vector<std::string>
{
    std::vector<std::string> endpoints;
    endpoints.reserve(server_list_.size());
    for (const auto& item : server_list_) {
        endpoints.emplace_back(item.address);
    }
    return endpoints;
}

auto
mcbp_multiplexer_state::get_pipeline(std::size_t index) const -> std::shared_ptr<mcbp_pipeline>
{
    // TODO(sergey): could it be ever negative?
    if (index >= pipelines_.size()) {
        return dead_pipeline_;
    }
    return pipelines_[index];
}

auto
mcbp_multiplexer_state::has_bucket_capability(bucket_capability capability) const -> bool
{
    return bucket_capabilities_.supports(capability);
}

auto
mcbp_multiplexer_state::debug_string() const -> std::string
{
    std::vector<char> out;
    for (std::size_t i = 0; i < pipelines_.size(); ++i) {
        fmt::format_to(std::back_insert_iterator(out), "pipeline#{}: {}\n", i, pipelines_[i]->debug_string());
    }
    if (dead_pipeline_ == nullptr) {
        fmt::format_to(std::back_insert_iterator(out), "pipeline#dead: disabled\n");
    } else {
        fmt::format_to(std::back_insert_iterator(out), "pipeline#dead: {}\n", dead_pipeline_->debug_string());
    }
    return { out.begin(), out.end() };
}

auto
mcbp_multiplexer_state::auth() const -> std::shared_ptr<auth_provider>
{
    return auth_provider_;
}

auto
mcbp_multiplexer_state::tls_config() const -> std::shared_ptr<tls_configuration>
{
    return tls_config_;
}
} // namespace couchbase::new_core
