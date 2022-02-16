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

namespace couchbase::new_core
{
class hello_properties
{
    bool mutation_tokens_enabled;
    bool collections_enabled;
    bool compression_enabled;
    bool durations_enabled;
    bool out_of_order_enabled;
    bool json_enabled;
    bool xerror_enabled;
    bool sync_replication_enabled;
    bool resource_units_enabled;
};
} // namespace couchbase::new_core
