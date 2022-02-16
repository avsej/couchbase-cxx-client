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
 * represents a memcached response status.
 */
enum class status_code : std::uint8_t {
    // indicates the operation completed successfully.
    success = 0x00,

    // occurs when an operation is performed on a key that does not exist.
    key_not_found = 0x01,

    // occurs when an operation is performed on a key that could not be found.
    key_exists = 0x02,

    // occurs when an operation attempts to store more data in a single document than the server is capable of storing (by default, this is
    // a 20MB limit).
    too_big = 0x03,

    // occurs when the server receives invalid arguments for an operation.
    invalid_argument = 0x04,

    // occurs when the server fails to store a key.
    not_store = 0x05,

    // occurs when an invalid delta value is specified to a counter operation.
    bad_delta = 0x06,

    // occurs when an operation is dispatched to a server which is non-authoritative for a specific vbucket.
    not_my_vbucket = 0x07,

    // occurs when no bucket was selected on a connection.
    no_bucket = 0x08,

    // occurs when an operation fails due to the document being locked.
    locked = 0x09,

    // occurs when authentication credentials have become invalidated.
    auth_stale = 0x1f,

    // occurs when the authentication information provided was not valid.
    auth_error = 0x20,

    // occurs in multi-step authentication when more authentication work needs to be performed in order to complete the authentication
    // process.
    auth_continue = 0x21,

    // occurs when the range specified to the server is not valid.
    range_error = 0x22,

    // occurs when a DCP stream fails to open due to a rollback having previously occurred since the last time the stream was opened.
    rollback = 0x23,

    // occurs when an access error occurs.
    access_error = 0x24,

    // is sent by servers which are still initializing, and are not yet ready to accept operations on behalf of a particular bucket.
    not_initialized = 0x25,

    // occurs when the server rate limits due to network ingress.
    rate_limited_network_ingress = 0x30,

    // occurs when the server rate limits due to network egress.
    rate_limited_network_egress = 0x31,

    // occurs when the server rate limits due to the application reaching the maximum number of allowed connections.
    rate_limited_max_connections = 0x32,

    // occurs when the server rate limits due to the application reaching the maximum number of allowed operations.
    rate_limited_max_commands = 0x33,

    // occurs when the server rate limits due to the application reaching the maximum data size allowed for the scope.
    rate_limited_scope_size_limit_exceeded = 0x34,

    // occurs when an unknown operation is sent to a server.
    unknown_command = 0x81,

    // occurs when the server cannot service a request due to memory limitations.
    out_of_memory = 0x82,

    // occurs when an operation is understood by the server, but that operation is not supported on this server (occurs for a variety of
    // reasons).
    not_supported = 0x83,

    // occurs when internal errors prevent the server from processing your request.
    internal_error = 0x84,

    // occurs when the server is too busy to process your request right away. Attempting the operation at a later time will likely succeed.
    busy = 0x85,

    // occurs when a temporary failure is preventing the server from
    // processing your request.
    temp_failure = 0x86,

    // occurs when a Collection cannot be found.
    collection_unknown = 0x88,

    // occurs when a Scope cannot be found.
    scope_unknown = 0x8c,

    // occurs when a dcp stream ID is invalid.
    dcp_stream_id_invalid = 0x8d,

    // occurs when an invalid durability level was requested.
    durability_invalid_level = 0xa0,

    // occurs when a request is performed with impossible durability level requirements.
    durability_impossible = 0xa1,

    // occurs when an attempt is made to write to a key that has a SyncWrite pending.
    sync_write_in_progress = 0xa2,

    // occurs when an SyncWrite does not complete in the specified time and the result is ambiguous.
    sync_write_ambiguous = 0xa3,

    // occurs when an SyncWrite is being recommitted.
    sync_write_re_commit_in_progress = 0xa4,

    // occurs when a sub-document operation targets a path which does not exist in the specifie document.
    subdoc_path_not_found = 0xc0,

    // occurs when a sub-document operation specifies a path which does not match the document structure (field access on an array).
    subdoc_path_mismatch = 0xc1,

    // occurs when a sub-document path could not be parsed.
    subdoc_path_invalid = 0xc2,

    // occurs when a sub-document path is too big.
    subdoc_path_too_big = 0xc3,

    // occurs when an operation would cause a document to be nested beyond the depth limits allowed by the sub-document specification.
    subdoc_path_too_deep = 0xc4,

    // occurs when a sub-document operation could not insert.
    subdoc_cannot_insert = 0xc5,

    // occurs when a sub-document operation is performed on a document which is not JSON.
    subdoc_not_json = 0xc6,

    // occurs when a sub-document operation is performed with a bad range.
    subdoc_bad_range = 0xc7,

    // occurs when a sub-document counter operation is performed and the specified delta is not valid.
    subdoc_bad_delta = 0xc8,

    // occurs when a sub-document operation expects a path not to exists, but the path was found in the document.
    subdoc_path_exists = 0xc9,

    // occurs when a sub-document operation specifies a value which is deeper than the depth limits of the sub-document specification.
    subdoc_value_too_deep = 0xca,

    // occurs when a multi-operation sub-document operation is performed and operations within the package of ops conflict with each other.
    subdoc_bad_combo = 0xcb,

    // occurs when a multi-operation sub-document operation is performed and operations within the package of ops conflict with each other.
    subdoc_bad_multi = 0xcc,

    // occurs when a multi-operation sub-document operation is performed on a soft-deleted document.
    subdoc_success_deleted = 0xcd,

    // occurs when an invalid set of extended-attribute flags is passed to a sub-document operation.
    subdoc_xattr_invalid_flag_combo = 0xce,

    // occurs when an invalid set of key operations are specified for a extended-attribute sub-document operation.
    subdoc_xattr_invalid_key_combo = 0xcf,

    // occurs when an invalid macro value is specified.
    subdoc_xattr_unknown_macro = 0xd0,

    // occurs when an invalid virtual attribute is specified.
    subdoc_xattr_unknown_vattr = 0xd1,

    // occurs when a mutation is attempted upon a virtual attribute (which are immutable by definition).
    subdoc_xattr_cannot_modify_vattr = 0xd2,

    // occurs when a Multi Path Failure occurs on a soft-deleted document.
    subdoc_multi_path_failure_deleted = 0xd3,
};

[[nodiscard]] std::string
to_string(status_code status);
} // namespace couchbase::new_core::mcbp
