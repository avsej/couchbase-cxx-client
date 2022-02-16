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

#include <system_error>

namespace couchbase::core
{
enum class core_errc {
    invalid_vbucket = 1201,
    invalid_replica = 1202,
    invalid_server = 1203,
    client_internal_error = 1204,
    no_cccp_hosts = 1205,
    /* FIXME: use global code here */
    request_cancelled = 1206,
    /* FIXME: use global code here */
    shutdown = 1207,
    /* FIXME: use global code here */
    parsing_failure = 1208,
    operation_queue_closed = 1209,
    operation_queue_full = 1210,
    request_already_queued = 1211,
    pipeline_closed = 1212,
    pipeline_full = 1213,
    short_write = 1214,
    end_of_file = 1215,
};

namespace detail
{
struct core_error_category : std::error_category {
    [[nodiscard]] const char* name() const noexcept override
    {
        return "couchbase.core";
    }

    [[nodiscard]] std::string message(int ev) const noexcept override
    {
        switch (static_cast<core_errc>(ev)) {
            case core_errc::invalid_vbucket:
                return "invalid_vbucket (1201)";
            case core_errc::invalid_replica:
                return "invalid_replica (1202)";
            case core_errc::invalid_server:
                return "invalid_server (1203)";
            case core_errc::client_internal_error:
                return "client_internal_error (1204)";
            case core_errc::no_cccp_hosts:
                return "no_cccp_hosts (1205)";
            case core_errc::request_cancelled:
                return "request_cancelled (1206)";
            case core_errc::shutdown:
                return "shutdown (1207)";
            case core_errc::parsing_failure:
                return "parsing_falure (1208)";
            case core_errc::operation_queue_closed:
                return "operation_queue_closed (1209)";
            case core_errc::operation_queue_full:
                return "operation_queue_full (1210)";
            case core_errc::request_already_queued:
                return "request_already_queued (1211)";
            case core_errc::pipeline_closed:
                return "pipeline_closed (1212)";
            case core_errc::pipeline_full:
                return "pipeline_full (1213)";
            case core_errc::short_write:
                return "pipeline_full (1214)";
            case core_errc::end_of_file:
                return "pipeline_full (1215)";
        }
        return "FIXME: unknown core error code (recompile with newer library)";
    }
};

const std::error_category&
get_core_category();

} // namespace detail

inline std::error_code
make_error_code(couchbase::core::core_errc e)
{
    return { static_cast<int>(e), couchbase::core::detail::get_core_category() };
}
} // namespace couchbase::core

namespace std
{
template<>
struct is_error_code_enum<couchbase::core::core_errc> : true_type {
};
} // namespace std
