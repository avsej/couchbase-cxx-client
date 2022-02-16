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

#include <string>

namespace couchbase::new_core
{
enum class retry_reason {
    // indicates that the operation failed for an unknown reason.
    unknown,

    // indicates that the operation failed because the underlying socket was not available.
    socket_not_available,

    // indicates that the operation failed because the requested service was not available.
    service_not_available,

    // indicates that the operation failed because the requested node was not available.
    node_not_available,

    // indicates that the operation failed because it was sent to the wrong node for the vbucket.
    key_value_not_my_vbucket,

    // indicates that the operation failed because the collection ID on the request is outdated.
    key_value_collection_outdated,

    // indicates that the operation failed for an unsupported reason but the KV error map indicated that the operation can be retried.
    key_value_error_map_retry,

    // indicates that the operation failed because the document was locked.
    key_value_locked,

    // indicates that the operation failed because of a temporary failure.
    key_value_temporary_failure,

    // indicates that the operation failed because a sync write is in progress.
    key_value_sync_write_in_progress,

    // indicates that the operation failed because a sync write recommit is in progress.
    key_value_sync_write_recommit_in_progress,

    // indicates that the operation failed and the service responded stating that the request should be retried.
    service_response_code_indicated,

    // indicates that the operation failed because the socket was closed whilst the operation was in flight.
    socket_close_in_flight,

    // indicates that the operation failed because the pipeline queue was full.
    pipeline_overloaded,

    // indicates that the operation failed because the circuit breaker for the underlying socket was open.
    circuit_breaker_open,

    // indicates that the operation failed to to a missing query index
    query_index_not_found,

    // indicates that the operation failed due to a prepared statement failure
    query_prepared_statement_failure,

    // indicates that the operation is retryable as indicated by the query engine.
    query_error_retryable,

    // indicates that an analytics operation failed due to a temporary failure
    analytics_temporary_failure,

    // indicates that a search operation failed due to too many requests
    search_too_many_requests,

    // indicates that the WaitUntilReady operation is not ready
    not_ready_retry_reason,

    // indicates that there was no pipeline snapshot available
    no_pipeline_snapshot,

    // indicates that the user has priviledges to access the bucket but the bucket doesn't exist or is in warm up.
    // Uncommitted: This API may change in the future.
    bucket_not_ready,

    // indicates that there were errors reported by underlying connections. Check server ports and cluster encryption setting.
    connection_error,

    // indicates that the operation failed because the write failed on the connection.
    mcbp_write_failure,
};

bool
allows_non_idempotent_retry(retry_reason reason);

bool
always_retry(retry_reason reason);

std::string
to_string(retry_reason reason);
} // namespace couchbase::new_core
