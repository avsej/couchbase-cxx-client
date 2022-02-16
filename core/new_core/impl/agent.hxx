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

#include "../agent_configuration.hxx"
#include "../auth_provider.hxx"
#include "../diagnostics_options.hxx"
#include "../ops_fwd.hxx"

#include <memory>
#include <mutex>

namespace couchbase::new_core::impl
{
class mcbp_multiplexer;
class http_multiplexer;
class config_management_component;
class error_map_component;
class collections_component;
class tracer_component;
class http_component;
class diagnostics_component;
class crud_component;
class observe_component;
class stats_component;
class query_component;
class analytics_component;
class search_component;
class view_component;
class orphan_logger_component;

class agent : public std::enable_shared_from_this<agent>
{
  public:
    explicit agent(const agent_configuration& config);

    [[nodiscard]] auto execute(ops::get_operation operation) -> completion_token;

    [[nodiscard]] auto diagnostics(const diagnostics_options& options) -> completion_token;

  private:
    std::string client_id_;
    std::string bucket_name_;
    std::shared_ptr<retry_strategy> default_retry_strategy_;
    std::shared_ptr<mcbp_multiplexer> mcbp_multiplexer_;
    std::shared_ptr<http_multiplexer> http_multiplexer_;

    std::shared_ptr<config_management_component> config_management_component_;
    std::shared_ptr<error_map_component> error_map_component_;
    std::shared_ptr<collections_component> collections_component_;
    std::shared_ptr<tracer_component> tracer_component_;
    std::shared_ptr<http_component> http_component_;
    std::shared_ptr<diagnostics_component> diagnostics_component_;
    std::shared_ptr<crud_component> crud_component_;
    std::shared_ptr<observe_component> observe_component_;
    // std::shared_ptr<stats_component> stats_component_;
    std::shared_ptr<query_component> query_component_;
    std::shared_ptr<analytics_component> analytics_component_;
    std::shared_ptr<search_component> search_component_;
    std::shared_ptr<view_component> view_component_;
    std::shared_ptr<orphan_logger_component> orphan_logger_component_;

    std::mutex connection_settings_mutex_;
    std::shared_ptr<auth_provider> auth_;
};
} // namespace couchbase::new_core::impl
