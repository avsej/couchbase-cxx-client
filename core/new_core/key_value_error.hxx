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

#include "mcbp/status_code.hxx"
#include "retry_reason.hxx"

#include <set>
#include <system_error>

namespace couchbase::new_core
{
struct key_value_error {
    std::error_code inner_error{};
    mcbp::status_code status{};
    std::string document_key{};
    std::string bucket_name{};
    std::string scope_name{};
    std::string collection_name{};
    std::uint32_t collection_id{};
    std::string error_name{};
    std::string error_description{};
    std::uint32_t opaque{};
    std::string error_context{};
    std::string error_reference{};
    std::set<retry_reason> retry_reasons{};
    std::size_t retry_attempts{};
    std::string last_dispatched_to{};
    std::string last_dispatched_from{};
    std::string last_connection_id{};
};
} // namespace couchbase::new_core
