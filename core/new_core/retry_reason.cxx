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

#include "retry_reason.hxx"

namespace couchbase::new_core
{
bool
allows_non_idempotent_retry(retry_reason reason)
{
    switch (reason) {
        case retry_reason::socket_not_available:
        case retry_reason::service_not_available:
        case retry_reason::node_not_available:
        case retry_reason::key_value_not_my_vbucket:
        case retry_reason::key_value_collection_outdated:
        case retry_reason::key_value_error_map_retry:
        case retry_reason::key_value_locked:
        case retry_reason::key_value_temporary_failure:
        case retry_reason::key_value_sync_write_in_progress:
        case retry_reason::key_value_sync_write_recommit_in_progress:
        case retry_reason::service_response_code_indicated:
        case retry_reason::pipeline_overloaded:
        case retry_reason::circuit_breaker_open:
        case retry_reason::query_index_not_found:
        case retry_reason::query_prepared_statement_failure:
        case retry_reason::query_error_retryable:
        case retry_reason::analytics_temporary_failure:
        case retry_reason::search_too_many_requests:
        case retry_reason::not_ready_retry_reason:
        case retry_reason::no_pipeline_snapshot:
        case retry_reason::bucket_not_ready:
        case retry_reason::connection_error:
        case retry_reason::mcbp_write_failure:
            return true;
        case retry_reason::unknown:
        case retry_reason::socket_close_in_flight:
            return false;
    }
    return false;
}

bool
always_retry(retry_reason reason)
{
    switch (reason) {
        case retry_reason::unknown:
        case retry_reason::socket_not_available:
        case retry_reason::service_not_available:
        case retry_reason::node_not_available:
        case retry_reason::key_value_error_map_retry:
        case retry_reason::key_value_locked:
        case retry_reason::key_value_temporary_failure:
        case retry_reason::key_value_sync_write_in_progress:
        case retry_reason::key_value_sync_write_recommit_in_progress:
        case retry_reason::service_response_code_indicated:
        case retry_reason::socket_close_in_flight:
        case retry_reason::circuit_breaker_open:
        case retry_reason::query_index_not_found:
        case retry_reason::query_prepared_statement_failure:
        case retry_reason::query_error_retryable:
        case retry_reason::analytics_temporary_failure:
        case retry_reason::search_too_many_requests:
        case retry_reason::no_pipeline_snapshot:
        case retry_reason::bucket_not_ready:
        case retry_reason::connection_error:
            return false;

        case retry_reason::key_value_not_my_vbucket:
        case retry_reason::key_value_collection_outdated:
        case retry_reason::pipeline_overloaded:
        case retry_reason::not_ready_retry_reason:
        case retry_reason::mcbp_write_failure:
            return true;
    }
    return false;
}

std::string
to_string(retry_reason reason)
{
    switch (reason) {
        case retry_reason::unknown:
            return "unknown";
        case retry_reason::socket_not_available:
            return "socket_not_available";
        case retry_reason::service_not_available:
            return "service_not_available";
        case retry_reason::node_not_available:
            return "node_not_available";
        case retry_reason::key_value_not_my_vbucket:
            return "key_value_not_my_vbucket";
        case retry_reason::key_value_collection_outdated:
            return "key_value_collection_outdated";
        case retry_reason::key_value_error_map_retry:
            return "key_value_error_map_retry";
        case retry_reason::key_value_locked:
            return "key_value_locked";
        case retry_reason::key_value_temporary_failure:
            return "key_value_temporary_failure";
        case retry_reason::key_value_sync_write_in_progress:
            return "key_value_sync_write_in_progress";
        case retry_reason::key_value_sync_write_recommit_in_progress:
            return "key_value_sync_write_recommit_in_progress";
        case retry_reason::service_response_code_indicated:
            return "service_response_code_indicated";
        case retry_reason::socket_close_in_flight:
            return "socket_close_in_flight";
        case retry_reason::pipeline_overloaded:
            return "pipeline_overloaded";
        case retry_reason::circuit_breaker_open:
            return "circuit_breaker_open";
        case retry_reason::query_index_not_found:
            return "query_index_not_found";
        case retry_reason::query_prepared_statement_failure:
            return "query_prepared_statement_failure";
        case retry_reason::query_error_retryable:
            return "query_error_retryable";
        case retry_reason::analytics_temporary_failure:
            return "analytics_temporary_failure";
        case retry_reason::search_too_many_requests:
            return "search_too_many_requests";
        case retry_reason::not_ready_retry_reason:
            return "not_ready_retry_reason";
        case retry_reason::no_pipeline_snapshot:
            return "no_pipeline_snapshot";
        case retry_reason::bucket_not_ready:
            return "bucket_not_ready";
        case retry_reason::connection_error:
            return "connection_error";
        case retry_reason::mcbp_write_failure:
            return "mcbp_write_failure";
    }
    return "invalid";
}

} // namespace couchbase::new_core
