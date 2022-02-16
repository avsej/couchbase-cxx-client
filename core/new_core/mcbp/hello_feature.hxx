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

namespace couchbase::new_core::mcbp
{
/**
 * HelloFeature represents a feature code included in a memcached HELLO operation.
 */
enum class hello_feature : std::uint16_t {
    // indicates support for Datatype fields.
    data_type = 0x01,

    // indicates support for TLS
    tls = 0x02,

    // indicates support for TCP no-delay.
    tcp_no_delay = 0x03,

    // indicates support for mutation tokens
    sequence_number = 0x04,

    // indicates support for TCP delay
    tcp_delay = 0x05,

    // indicates support for document xattrs
    xattr = 0x06,

    // indicates support for extended errors.
    xerror = 0x07,

    // indicates support for the SelectBucket operation.
    select_bucket = 0x08,

    // 0x09 is reserved and cannot be used.

    // indicates support for snappy compressed documents.
    snappy = 0x0a,

    // indicates support for JSON datatype data.
    json = 0x0b,

    // indicates support for duplex communications.
    duplex = 0x0c,

    // indicates support for cluster-map update notifications.
    cluster_map_notification = 0x0d,

    // indicates support for unordered execution of operations.
    unordered_execution = 0x0e,

    // indicates support for server durations.
    durations = 0x0f,

    // indicates support for requests with flexible frame extras.
    alt_requests = 0x10,

    // indicates support for requests synchronous durability requirements.
    sync_replication = 0x11,

    // indicates support for collections.
    collections = 0x12,

    // indicates support for OpenTracing.
    open_tracing = 0x13,

    // indicates support for preserve TTL.
    preserve_expiry = 0x14,

    // indicates support for PiTR snapshots.
    pitr = 0x16,

    // indicates support for the create as deleted feature.
    create_as_deletec = 0x17,

    // indicates support for the replace body with xattr feature.
    replace_body_with_xattr = 0x19,

    resource_units = 0x1a,
};
} // namespace couchbase::new_core::mcbp
