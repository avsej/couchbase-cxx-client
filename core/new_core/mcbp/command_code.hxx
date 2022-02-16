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

#include <cinttypes>
#include <string>

namespace couchbase::new_core::mcbp
{
/**
 * Represents the specific command the packet is performing.
 */
enum class command_code : std::uint8_t {
    get = 0x00,
    set = 0x01,
    add = 0x02,
    replace = 0x03,
    remove = 0x04,
    increment = 0x05,
    decrement = 0x06,
    noop = 0x0a,
    append = 0x0e,
    prepend = 0x0f,
    stat = 0x10,
    touch = 0x1c,
    get_and_touch = 0x1d,
    hello = 0x1f,
    sasl_list_mechanisms = 0x20,
    sasl_authenticate = 0x21,
    sasl_step = 0x22,
    get_all_vb_sequence_numbers = 0x48,
    dcp_open_connection = 0x50,
    dcp_add_stream = 0x51,
    dcp_close_stream = 0x52,
    dcp_stream_request = 0x53,
    dcp_get_failover_log = 0x54,
    dcp_stream_end = 0x55,
    dcp_snapshot_marker = 0x56,
    dcp_mutation = 0x57,
    dcp_deletion = 0x58,
    dcp_expiration = 0x59,
    dcp_sequence_number_advanced = 0x64,
    dcp_oso_snapshot = 0x65,
    dcp_flush = 0x5a,
    dcp_set_vbucket_state = 0x5b,
    dcp_noop = 0x5c,
    dcp_buffer_ack = 0x5d,
    dcp_control = 0x5e,
    dcp_event = 0x5f,
    get_replica = 0x83,
    select_bucket = 0x89,
    observe_sequence_number = 0x91,
    observe = 0x92,
    get_locked = 0x94,
    unlock_key = 0x95,
    get_meta = 0xa0,
    set_meta = 0xa2,
    remove_meta = 0xa8,
    get_cluster_config = 0xb5,
    get_random = 0xb6,
    collections_get_manifest = 0xba,
    collections_get_id = 0xbb,
    subdoc_get = 0xc5,
    subdoc_exist = 0xc6,
    subdoc_dict_add = 0xc7,
    subdoc_dict_set = 0xc8,
    subdoc_remove = 0xc9,
    subdoc_replace = 0xca,
    subdoc_array_push_last = 0xcb,
    subdoc_array_push_first = 0xcc,
    subdoc_array_insert = 0xcd,
    subdoc_array_add_unique = 0xce,
    subdoc_counter = 0xcf,
    subdoc_multi_lookup = 0xd0,
    subdoc_multi_mutation = 0xd1,
    subdoc_get_count = 0xd2,
    subdoc_replace_body_with_xattr = 0xd3,
    get_error_map = 0xfe,
    invalid = 0xff,
};

[[nodiscard]] auto
is_idempotent(command_code opcode) -> bool;

[[nodiscard]] auto
to_string(command_code opcode) -> std::string;

[[nodiscard]] auto
supports_collection_id(mcbp::command_code command) -> bool;

} // namespace couchbase::new_core::mcbp
