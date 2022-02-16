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

#include "status_code.hxx"

namespace couchbase::new_core::mcbp
{
std::string
to_string(status_code status)
{
    switch (status) {
        case status_code::success:
            return "success";
        case status_code::key_not_found:
            return "key_not_found";
        case status_code::key_exists:
            return "key_exists";
        case status_code::too_big:
            return "too_big";
        case status_code::invalid_argument:
            return "invalid_argument";
        case status_code::not_store:
            return "not_store";
        case status_code::bad_delta:
            return "bad_delta";
        case status_code::not_my_vbucket:
            return "not_my_vbucket";
        case status_code::no_bucket:
            return "no_bucket";
        case status_code::locked:
            return "locked";
        case status_code::auth_stale:
            return "auth_stale";
        case status_code::auth_error:
            return "auth_error";
        case status_code::auth_continue:
            return "auth_continue";
        case status_code::range_error:
            return "range_error";
        case status_code::rollback:
            return "rollback";
        case status_code::access_error:
            return "access_error";
        case status_code::not_initialized:
            return "not_initialized";
        case status_code::rate_limited_network_ingress:
            return "rate_limited_network_ingress";
        case status_code::rate_limited_network_egress:
            return "rate_limited_network_egress";
        case status_code::rate_limited_max_connections:
            return "rate_limited_max_connections";
        case status_code::rate_limited_max_commands:
            return "rate_limited_max_commands";
        case status_code::rate_limited_scope_size_limit_exceeded:
            return "rate_limited_scope_size_limit_exceeded";
        case status_code::unknown_command:
            return "unknown_command";
        case status_code::out_of_memory:
            return "out_of_memory";
        case status_code::not_supported:
            return "not_supported";
        case status_code::internal_error:
            return "internal_error";
        case status_code::busy:
            return "busy";
        case status_code::temp_failure:
            return "temp_failure";
        case status_code::collection_unknown:
            return "collection_unknown";
        case status_code::scope_unknown:
            return "scope_unknown";
        case status_code::dcp_stream_id_invalid:
            return "dcp_stream_id_invalid";
        case status_code::durability_invalid_level:
            return "durability_invalid_level";
        case status_code::durability_impossible:
            return "durability_impossible";
        case status_code::sync_write_in_progress:
            return "sync_write_in_progress";
        case status_code::sync_write_ambiguous:
            return "sync_write_ambiguous";
        case status_code::sync_write_re_commit_in_progress:
            return "sync_write_re_commit_in_progress";
        case status_code::subdoc_path_not_found:
            return "subdoc_path_not_found";
        case status_code::subdoc_path_mismatch:
            return "subdoc_path_mismatch";
        case status_code::subdoc_path_invalid:
            return "subdoc_path_invalid";
        case status_code::subdoc_path_too_big:
            return "subdoc_path_too_big";
        case status_code::subdoc_path_too_deep:
            return "subdoc_path_too_deep";
        case status_code::subdoc_cannot_insert:
            return "subdoc_cannot_insert";
        case status_code::subdoc_not_json:
            return "subdoc_not_json";
        case status_code::subdoc_bad_range:
            return "subdoc_bad_range";
        case status_code::subdoc_bad_delta:
            return "subdoc_bad_delta";
        case status_code::subdoc_path_exists:
            return "subdoc_path_exists";
        case status_code::subdoc_value_too_deep:
            return "subdoc_value_too_deep";
        case status_code::subdoc_bad_combo:
            return "subdoc_bad_combo";
        case status_code::subdoc_bad_multi:
            return "subdoc_bad_multi";
        case status_code::subdoc_success_deleted:
            return "subdoc_success_deleted";
        case status_code::subdoc_xattr_invalid_flag_combo:
            return "subdoc_xattr_invalid_flag_combo";
        case status_code::subdoc_xattr_invalid_key_combo:
            return "subdoc_xattr_invalid_key_combo";
        case status_code::subdoc_xattr_unknown_macro:
            return "subdoc_xattr_unknown_macro";
        case status_code::subdoc_xattr_unknown_vattr:
            return "subdoc_xattr_unknown_vattr";
        case status_code::subdoc_xattr_cannot_modify_vattr:
            return "subdoc_xattr_cannot_modify_vattr";
        case status_code::subdoc_multi_path_failure_deleted:
            return "subdoc_multi_path_failure_deleted";
    }
    return "invalid";
}
} // namespace couchbase::new_core::mcbp
