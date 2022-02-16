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

#include "circuit_breaker_callback.hxx"
#include "circuit_breaker_state.hxx"

#include <chrono>

namespace couchbase::new_core
{
class circuit_breaker
{
  public:
    virtual ~circuit_breaker() = default;

    [[nodiscard]] virtual auto allows_request() -> bool = 0;
    virtual void mark_successful() = 0;
    virtual void mark_failure() = 0;
    [[nodiscard]] virtual auto state() -> circuit_breaker_state = 0;
    virtual void reset() = 0;
    [[nodiscard]] virtual auto canary_timeout() -> std::chrono::nanoseconds = 0;
    [[nodiscard]] virtual auto invoke_callback(std::error_code error) -> bool = 0;
};
} // namespace couchbase::new_core
