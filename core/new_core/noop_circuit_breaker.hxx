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

#include "circuit_breaker.hxx"

namespace couchbase::new_core
{
class noop_circuit_breaker : public circuit_breaker
{
  public:
    ~noop_circuit_breaker() override = default;

    auto allows_request() -> bool override
    {
        return true;
    }

    void mark_successful() override
    {
        /* do nothing */
    }

    void mark_failure() override
    {
        /* do nothing */
    }

    auto state() -> circuit_breaker_state override
    {
        return circuit_breaker_state::disabled;
    }

    void reset() override
    {
        /* do nothing */
    }

    auto canary_timeout() -> std::chrono::nanoseconds override
    {
        return {};
    }

    auto invoke_callback(std::error_code /* error */) -> bool override
    {
        return true;
    }
};

auto
make_noop_circuit_breaker() -> std::shared_ptr<noop_circuit_breaker>;
} // namespace couchbase::new_core
