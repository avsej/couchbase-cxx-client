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
#include <string>

namespace couchbase::new_core
{
struct mcbp_client_dialer_properties {
    std::chrono::milliseconds key_value_connect_timeout;
    std::chrono::milliseconds server_wait_timeout;
    std::string client_id;
    std::size_t compression_min_size;
    double compression_min_ratio;
    bool disable_compression;
    bool no_tls_seed_node;
};
} // namespace couchbase::new_core
