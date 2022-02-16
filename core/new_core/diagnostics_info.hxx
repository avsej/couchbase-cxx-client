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

#include "cluster_state.hxx"
#include "mcbp_connection_info.hxx"

#include <cinttypes>
#include <vector>

namespace couchbase::new_core
{
// is returned by the diagnostics() method and includes information about the overall health of the clients connections.
struct diagnostics_info {
    std::uint64_t revision;
    cluster_state state;
    std::vector<mcbp_connection_info> mcbp_connections;
};
} // namespace couchbase::new_core
