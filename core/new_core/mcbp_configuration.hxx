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

#include <chrono>

namespace couchbase::new_core
{
// specifies kv related configuration options.
struct mcbp_configuration {
    // the timeout value to apply when dialling tcp connections.
    std::chrono::milliseconds connect_timeout;

    // the period of time that the SDK will wait before reattempting connection to a node after
    // bootstrap fails against that node.
    std::chrono::milliseconds server_wait_backoff;

    // the number of connections to create to each node.
    std::size_t pool_size;

    // the maximum number of requests that can be queued waiting to be sent to a node.
    std::size_t max_queue_size;
};
} // namespace couchbase::new_core
