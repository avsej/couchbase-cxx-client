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
#include "circuit_breaker_configuration.hxx"

#include <atomic>
#include <cstdint>
#include <memory>

namespace couchbase::new_core
{
class lazy_circuit_breaker : public circuit_breaker
{
  public:
    lazy_circuit_breaker(const circuit_breaker_config& config, std::function<void()> send_canary);
    ~lazy_circuit_breaker() override = default;

    auto allows_request() -> bool override;
    void mark_successful() override;
    void mark_failure() override;
    auto state() -> circuit_breaker_state override;
    void reset() override;
    auto canary_timeout() -> std::chrono::nanoseconds override;
    auto invoke_callback(std::error_code error) -> bool override;

  private:
    void maybe_reset_rolling_window();
    void maybe_open_circuit();

    std::chrono::nanoseconds sleep_window_;
    std::chrono::nanoseconds rolling_window_;
    std::uint64_t volume_threshold_;
    double error_percentage_threshold_;
    std::chrono::nanoseconds canary_timeout_;
    circuit_breaker_callback completion_callback_;
    std::function<void()> send_canary_;

    std::atomic<circuit_breaker_state> state_{ circuit_breaker_state::closed };
    std::atomic<std::uint64_t> total_{ 0 };
    std::atomic<std::uint64_t> failed_{ 0 };
    std::atomic<std::chrono::nanoseconds> opened_at_{ std::chrono::nanoseconds::zero() };
    std::atomic<std::chrono::nanoseconds> window_start_{ std::chrono::steady_clock::now().time_since_epoch() };
};

auto
make_lazy_circuit_breaker(const circuit_breaker_config& config, std::function<void()> send_canary) -> std::shared_ptr<lazy_circuit_breaker>;
} // namespace couchbase::new_core
