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

#include "endpoint_state.hxx"

#include <chrono>
#include <string>

namespace couchbase::new_core
{
// represents information we know about a particular memcached connection reported in a diagnostics report.
struct mcbp_connection_info {
    std::string id;
    std::string local_address;
    std::string remote_address;
    std::string scope;
    endpoint_state state;
    std::chrono::steady_clock::time_point last_activity;
};
} // namespace couchbase::new_core
