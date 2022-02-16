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

namespace couchbase::new_core
{
// the current state of an endpoint used in a ping_result.
enum class ping_state : std::uint32_t {
    // indicates that an endpoint is OK.
    ok = 1,

    // indicates that the ping request to an endpoint timed out.
    timeout = 2,

    // indicates that the ping request to an endpoint encountered an error.
    error = 3,
};
} // namespace couchbase::new_core
