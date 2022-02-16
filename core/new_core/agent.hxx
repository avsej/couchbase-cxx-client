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

#include "agent_configuration.hxx"
#include "completion_token.hxx"
#include "diagnostics_options.hxx"
#include "impl/fwd/agent.hxx"
#include "ops.hxx"

#include <memory>

namespace couchbase::new_core
{
class agent
{
  public:
    explicit agent(const agent_configuration& config);
    agent(const agent& other) = default;
    agent(agent&& other) = default;
    auto operator=(const agent& other) -> agent& = default;
    auto operator=(agent&& other) -> agent& = default;

    [[nodiscard]] auto execute(ops::get_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::get_and_touch_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::get_and_lock_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::get_one_replica_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::touch_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::unlock_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::remove_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::insert_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::upsert_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::replace_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::append_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::prepend_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::increment_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::decrement_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::get_random_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::get_meta_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::set_meta_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::remove_meta_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::stats_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::observe_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::observe_vbucket_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::lookup_in_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::mutate_in_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::query_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::prepared_query_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::analytics_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::search_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::view_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::freeform_http_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::get_collection_manifest_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::get_all_collection_manifests_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::get_collection_id_operation) -> completion_token;
    [[nodiscard]] auto execute(ops::ping_operation) -> completion_token;

    [[nodiscard]] auto diagnostics(const diagnostics_options& options) -> completion_token;

  private:
    std::shared_ptr<impl::agent> impl_;
};
} // namespace couchbase::new_core
