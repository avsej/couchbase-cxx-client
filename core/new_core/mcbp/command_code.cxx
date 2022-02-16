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

#include "command_code.hxx"

namespace couchbase::new_core::mcbp
{
auto
is_idempotent(command_code opcode) -> bool
{
    switch (opcode) {
        case command_code::get:
        case command_code::noop:
        case command_code::get_replica:
        case command_code::observe_sequence_number:
        case command_code::observe:
        case command_code::get_meta:
        case command_code::stat:
        case command_code::get_cluster_config:
        case command_code::get_random:
        case command_code::collections_get_manifest:
        case command_code::collections_get_id:
        case command_code::subdoc_get:
        case command_code::subdoc_exist:
        case command_code::subdoc_multi_lookup:
        case command_code::subdoc_get_count:
            return true;
        default:
            break;
    }
    return false;
}

auto
to_string(command_code opcode) -> std::string
{
    switch (opcode) {
        case command_code::get:
            return "get";

        case command_code::set:
            return "set";

        case command_code::add:
            return "add";

        case command_code::replace:
            return "replace";

        case command_code::remove:
            return "remove";

        case command_code::increment:
            return "increment";

        case command_code::decrement:
            return "decrement";

        case command_code::noop:
            return "noop";

        case command_code::append:
            return "append";

        case command_code::prepend:
            return "prepend";

        case command_code::stat:
            return "stat";

        case command_code::touch:
            return "touch";

        case command_code::get_and_touch:
            return "get_and_touch";

        case command_code::hello:
            return "hello";

        case command_code::sasl_list_mechanisms:
            return "sasl_list_mechanisms";

        case command_code::sasl_authenticate:
            return "sasl_authenticate";

        case command_code::sasl_step:
            return "sasl_step";

        case command_code::get_all_vb_sequence_numbers:
            return "get_all_vb_sequence_numbers";

        case command_code::dcp_open_connection:
            return "dcp_open_connection";

        case command_code::dcp_add_stream:
            return "dcp_add_stream";

        case command_code::dcp_close_stream:
            return "dcp_close_stream";

        case command_code::dcp_stream_request:
            return "dcp_stream_request";

        case command_code::dcp_get_failover_log:
            return "dcp_get_failover_log";

        case command_code::dcp_stream_end:
            return "dcp_stream_end";

        case command_code::dcp_snapshot_marker:
            return "dcp_snapshot_marker";

        case command_code::dcp_mutation:
            return "dcp_mutation";

        case command_code::dcp_deletion:
            return "dcp_delection";

        case command_code::dcp_expiration:
            return "dcp_expiration";

        case command_code::dcp_sequence_number_advanced:
            return "dcp_sequence_number_advanced";

        case command_code::dcp_oso_snapshot:
            return "dcp_oso_snapshot";

        case command_code::dcp_flush:
            return "dcp_flush";

        case command_code::dcp_set_vbucket_state:
            return "dcp_set_vbucket_state";

        case command_code::dcp_noop:
            return "dcp_noop";

        case command_code::dcp_buffer_ack:
            return "dcp_buffer_ack";

        case command_code::dcp_control:
            return "dcp_control";

        case command_code::dcp_event:
            return "dcp_event";

        case command_code::get_replica:
            return "get_replica";

        case command_code::select_bucket:
            return "select_bucket";

        case command_code::observe_sequence_number:
            return "observe_sequence_number";

        case command_code::observe:
            return "observe";

        case command_code::get_locked:
            return "get_locked";

        case command_code::unlock_key:
            return "unlcok_key";

        case command_code::get_meta:
            return "get_meta";

        case command_code::set_meta:
            return "set_meta";

        case command_code::remove_meta:
            return "remove_meta";

        case command_code::get_cluster_config:
            return "get_cluster_config";

        case command_code::get_random:
            return "get_random";

        case command_code::collections_get_manifest:
            return "collections_get_manifest";

        case command_code::collections_get_id:
            return "collections_get_id";

        case command_code::subdoc_get:
            return "subdoc_get";

        case command_code::subdoc_exist:
            return "subdoc_exist";

        case command_code::subdoc_dict_add:
            return "subdoc_dict_add";

        case command_code::subdoc_dict_set:
            return "subdoc_dict_set";

        case command_code::subdoc_remove:
            return "subdoc_remove";

        case command_code::subdoc_replace:
            return "subdoc_replace";

        case command_code::subdoc_array_push_last:
            return "subdoc_array_push_last";

        case command_code::subdoc_array_push_first:
            return "subdoc_array_push_first";

        case command_code::subdoc_array_insert:
            return "subdoc_array_insert";

        case command_code::subdoc_array_add_unique:
            return "subdoc_array_add_unique";

        case command_code::subdoc_counter:
            return "subdoc_counter";

        case command_code::subdoc_multi_lookup:
            return "subdoc_multi_lookup";

        case command_code::subdoc_multi_mutation:
            return "subdoc_multi_mutation";

        case command_code::subdoc_get_count:
            return "subdoc_get_count";

        case command_code::subdoc_replace_body_with_xattr:
            return "subdoc_replace_body_with_xattr";

        case command_code::get_error_map:
            return "get_error_map";

        case command_code::invalid:
            return "invalid";
    }
    return "invalid";
}

bool
supports_collection_id(mcbp::command_code command)
{
    switch (command) {
        case mcbp::command_code::get:
        case mcbp::command_code::set:
        case mcbp::command_code::add:
        case mcbp::command_code::replace:
        case mcbp::command_code::remove:
        case mcbp::command_code::increment:
        case mcbp::command_code::decrement:
        case mcbp::command_code::append:
        case mcbp::command_code::prepend:
        case mcbp::command_code::touch:
        case mcbp::command_code::get_and_touch:
        case mcbp::command_code::dcp_mutation:
        case mcbp::command_code::dcp_deletion:
        case mcbp::command_code::dcp_expiration:
        case mcbp::command_code::get_replica:
        case mcbp::command_code::get_locked:
        case mcbp::command_code::unlock_key:
        case mcbp::command_code::get_meta:
        case mcbp::command_code::set_meta:
        case mcbp::command_code::remove_meta:
        case mcbp::command_code::subdoc_get:
        case mcbp::command_code::subdoc_exist:
        case mcbp::command_code::subdoc_dict_add:
        case mcbp::command_code::subdoc_dict_set:
        case mcbp::command_code::subdoc_remove:
        case mcbp::command_code::subdoc_replace:
        case mcbp::command_code::subdoc_array_push_last:
        case mcbp::command_code::subdoc_array_push_first:
        case mcbp::command_code::subdoc_array_insert:
        case mcbp::command_code::subdoc_array_add_unique:
        case mcbp::command_code::subdoc_counter:
        case mcbp::command_code::subdoc_multi_lookup:
        case mcbp::command_code::subdoc_multi_mutation:
        case mcbp::command_code::subdoc_get_count:
        case mcbp::command_code::subdoc_replace_body_with_xattr:
            return true;
        default:
            break;
    }
    return false;
}
} // namespace couchbase::new_core::mcbp
