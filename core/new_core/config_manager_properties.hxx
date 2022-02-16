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

#include "bucket_type.hxx"
#include "route_endpoint.hxx"

#include <string>
#include <vector>

namespace couchbase::new_core
{
struct config_manager_properties {
    bool use_tls{ false };
    bool no_tls_seed_node{ false };
    std::string network_type{ "default" };
    std::vector<route_endpoint> source_mcbp_addresses{};
    std::vector<route_endpoint> source_http_addresses{};
};
} // namespace couchbase::new_core
