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

#include <chrono>
#include <cstdint>

namespace couchbase::new_core
{
// The set of configuration settings for configuring circuit breakers.
// If enabled is set to false then a noop circuit breaker will be used, otherwise a lazy circuit breaker.
struct circuit_breaker_configuration {
    bool enabled{ false };

    // the minimum amount of operations to measure before the threshold percentage kicks in.
    std::uint64_t volume_threshold{ 20 };

    // the percentage of operations that need to fail in a window until the circuit opens.
    double error_threshold_percentage{ 50 };

    // the initial sleep time after which a canary is sent as a probe.
    std::chrono::nanoseconds sleep_window{ std::chrono::seconds{ 5 } };

    // the rolling timeframe which is used to calculate the error threshold percentage.
    std::chrono::nanoseconds rolling_window{ std::chrono::minutes{ 1 } };

    // the timeout for the canary request until it is deemed failed.
    std::chrono::nanoseconds canary_timeout{ std::chrono::seconds{ 5 } };

    // called on every response to determine if it is successful or not.
    circuit_breaker_callback completion_callback{ default_circuit_breaker_callback };
};

} // namespace couchbase::new_core
