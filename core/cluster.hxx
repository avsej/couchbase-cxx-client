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

#include "diagnostics.hxx"
#include "mcbp/queue_request.hxx"
#include "operations_fwd.hxx"
#include "origin.hxx"
#include "topology/configuration.hxx"
#include "utils/movable_function.hxx"

#include <asio/io_context.hpp>
#include <utility>

namespace couchbase::core
{
class crud_component;
class cluster_impl;

class cluster
{
  public:
    explicit cluster(asio::io_context& ctx);

    [[nodiscard]] auto io_context() const -> asio::io_context&;

    [[nodiscard]] std::pair<std::error_code, couchbase::core::origin> origin() const;

    void open(couchbase::core::origin origin, utils::movable_function<void(std::error_code)>&& handler) const;

    void close(utils::movable_function<void()>&& handler) const;

    void open_bucket(const std::string& bucket_name, utils::movable_function<void(std::error_code)>&& handler) const;

    void close_bucket(const std::string& bucket_name, utils::movable_function<void(std::error_code)>&& handler) const;

    void with_bucket_configuration(const std::string& bucket_name,
                                   utils::movable_function<void(std::error_code, topology::configuration)>&& handler) const;

#define DECLARE_OPERATION(name)                                                                                                            \
    void execute(operations::name##_request request, utils::movable_function<void(operations::name##_response)>&& handler) const

    DECLARE_OPERATION(analytics);
    DECLARE_OPERATION(append);
    DECLARE_OPERATION(decrement);
    DECLARE_OPERATION(exists);
    DECLARE_OPERATION(get);
    DECLARE_OPERATION(get_all_replicas);
    DECLARE_OPERATION(get_and_lock);
    DECLARE_OPERATION(get_and_touch);
    DECLARE_OPERATION(get_any_replica);
    DECLARE_OPERATION(get_projected);
    DECLARE_OPERATION(increment);
    DECLARE_OPERATION(insert);
    DECLARE_OPERATION(lookup_in);
    DECLARE_OPERATION(mutate_in);
    DECLARE_OPERATION(prepend);
    DECLARE_OPERATION(query);
    DECLARE_OPERATION(remove);
    DECLARE_OPERATION(replace);
    DECLARE_OPERATION(search);
    DECLARE_OPERATION(touch);
    DECLARE_OPERATION(unlock);
    DECLARE_OPERATION(upsert);
    DECLARE_OPERATION(document_view);
    DECLARE_OPERATION(http_noop);

    DECLARE_OPERATION(management::analytics_dataset_create);
    DECLARE_OPERATION(management::analytics_dataset_drop);
    DECLARE_OPERATION(management::analytics_dataset_get_all);
    DECLARE_OPERATION(management::analytics_dataverse_create);
    DECLARE_OPERATION(management::analytics_dataverse_drop);
    DECLARE_OPERATION(management::analytics_get_pending_mutations);
    DECLARE_OPERATION(management::analytics_index_create);
    DECLARE_OPERATION(management::analytics_index_drop);
    DECLARE_OPERATION(management::analytics_index_get_all);
    DECLARE_OPERATION(management::analytics_link_connect);
    DECLARE_OPERATION(management::analytics_link_disconnect);
    DECLARE_OPERATION(management::analytics_link_drop);
    DECLARE_OPERATION(management::analytics_link_get_all);
    DECLARE_OPERATION(management::bucket_create);
    DECLARE_OPERATION(management::bucket_drop);
    DECLARE_OPERATION(management::bucket_flush);
    DECLARE_OPERATION(management::bucket_get);
    DECLARE_OPERATION(management::bucket_get_all);
    DECLARE_OPERATION(management::bucket_update);
    DECLARE_OPERATION(management::collection_create);
    DECLARE_OPERATION(management::collection_drop);
    DECLARE_OPERATION(management::collections_manifest_get);
    DECLARE_OPERATION(management::scope_create);
    DECLARE_OPERATION(management::scope_drop);
    DECLARE_OPERATION(management::scope_get_all);
    DECLARE_OPERATION(management::eventing_deploy_function);
    DECLARE_OPERATION(management::eventing_drop_function);
    DECLARE_OPERATION(management::eventing_get_all_functions);
    DECLARE_OPERATION(management::eventing_get_function);
    DECLARE_OPERATION(management::eventing_get_status);
    DECLARE_OPERATION(management::eventing_pause_function);
    DECLARE_OPERATION(management::eventing_resume_function);
    DECLARE_OPERATION(management::eventing_undeploy_function);
    DECLARE_OPERATION(management::eventing_upsert_function);
    DECLARE_OPERATION(management::view_index_drop);
    DECLARE_OPERATION(management::view_index_get);
    DECLARE_OPERATION(management::view_index_get_all);
    DECLARE_OPERATION(management::view_index_upsert);
    DECLARE_OPERATION(management::change_password);
    DECLARE_OPERATION(management::group_drop);
    DECLARE_OPERATION(management::group_get);
    DECLARE_OPERATION(management::group_get_all);
    DECLARE_OPERATION(management::group_upsert);
    DECLARE_OPERATION(management::role_get_all);
    DECLARE_OPERATION(management::user_drop);
    DECLARE_OPERATION(management::user_get);
    DECLARE_OPERATION(management::user_get_all);
    DECLARE_OPERATION(management::user_upsert);
    DECLARE_OPERATION(management::search_get_stats);
    DECLARE_OPERATION(management::search_index_analyze_document);
    DECLARE_OPERATION(management::search_index_control_ingest);
    DECLARE_OPERATION(management::search_index_control_plan_freeze);
    DECLARE_OPERATION(management::search_index_control_query);
    DECLARE_OPERATION(management::search_index_drop);
    DECLARE_OPERATION(management::search_index_get);
    DECLARE_OPERATION(management::search_index_get_all);
    DECLARE_OPERATION(management::search_index_get_documents_count);
    DECLARE_OPERATION(management::search_index_get_stats);
    DECLARE_OPERATION(management::search_index_upsert);
    DECLARE_OPERATION(management::query_index_build);
    DECLARE_OPERATION(management::query_index_build_deferred);
    DECLARE_OPERATION(management::query_index_create);
    DECLARE_OPERATION(management::query_index_drop);
    DECLARE_OPERATION(management::query_index_get_all);
    DECLARE_OPERATION(management::query_index_get_all_deferred);

    DECLARE_OPERATION(management::freeform);
    DECLARE_OPERATION(management::bucket_describe);
    DECLARE_OPERATION(management::cluster_describe);
#undef DECLARE_OPERATION

#define DECLARE_OPERATION_IMPL(name)                                                                                                       \
    void execute(impl::name##_request request, utils::movable_function<void(impl::name##_response)>&& handler) const

    DECLARE_OPERATION_IMPL(get_replica);
    DECLARE_OPERATION_IMPL(observe_seqno);
#undef DECLARE_OPERATION_IMPL

#define DECLARE_ANALYTICS_LINK_OPERATION(name, type)                                                                                       \
    void execute(operations::management::analytics_link_##name##_request<management::analytics::type##_link> request,                      \
                 utils::movable_function<void(operations::management::analytics_link_##name##_response)>&& handler) const

    DECLARE_ANALYTICS_LINK_OPERATION(replace, azure_blob_external);
    DECLARE_ANALYTICS_LINK_OPERATION(replace, couchbase_remote);
    DECLARE_ANALYTICS_LINK_OPERATION(replace, s3_external);
    DECLARE_ANALYTICS_LINK_OPERATION(create, azure_blob_external);
    DECLARE_ANALYTICS_LINK_OPERATION(create, couchbase_remote);
    DECLARE_ANALYTICS_LINK_OPERATION(create, s3_external);
#undef DECLARE_ANALYTICS_LINK_OPERATION

    void diagnostics(std::optional<std::string> report_id, utils::movable_function<void(diag::diagnostics_result)>&& handler) const;

    void ping(std::optional<std::string> report_id,
              std::optional<std::string> bucket_name,
              std::set<service_type> services,
              utils::movable_function<void(diag::ping_result)>&& handler) const;

    auto direct_dispatch(const std::string& bucket_name, std::shared_ptr<couchbase::core::mcbp::queue_request> req) const
      -> std::error_code;

    auto direct_re_queue(const std::string& bucket_name, std::shared_ptr<mcbp::queue_request> req, bool is_retry) const -> std::error_code;

  private:
    std::shared_ptr<cluster_impl> impl_;
};
} // namespace couchbase::core
