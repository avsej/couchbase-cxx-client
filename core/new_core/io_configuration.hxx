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
// specifies IO related configuration options such as HELLO flags and the network type to use.
struct io_configuration {
    // defines which network to use from the cluster config.
    std::string network_type;

    bool enable_mutation_tokens;
    bool enable_out_of_order_responses;
    bool disable_xerror;
    bool disable_json;
    bool disable_sync_replication;
    bool enable_collections;
};
} // namespace couchbase::new_core
