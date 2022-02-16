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
// specifies http related configuration options.
struct http_configuration {
    // controls the maximum number of idle (keep-alive) connections across all hosts.
    std::size_t max_idle_connections;

    // controls the maximum idle (keep-alive) connections to keep per-host.
    std::size_t max_idle_connections_per_host;

    std::chrono::milliseconds connect_timeout;

    // the maximum amount of time an idle (keep-alive) connection will remain idle before closing itself.
    std::chrono::milliseconds idle_connection_timeout;
};
} // namespace couchbase::new_core
