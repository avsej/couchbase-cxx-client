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

#include "lazy_circuit_breaker.hxx"

#include "core/logger/logger.hxx"

namespace couchbase::new_core
{
auto
make_lazy_circuit_breaker(const circuit_breaker_config& config, std::function<void()> send_canary) -> std::shared_ptr<lazy_circuit_breaker>
{
    return std::make_shared<lazy_circuit_breaker>(config, std::move(send_canary));
}

lazy_circuit_breaker::lazy_circuit_breaker(const circuit_breaker_config& config, std::function<void()> send_canary)
  : sleep_window_{ config.sleep_window }
  , rolling_window_{ config.rolling_window }
  , volume_threshold_{ config.volume_threshold }
  , error_percentage_threshold_{ config.error_threshold_percentage }
  , canary_timeout_{ config.canary_timeout }
  , completion_callback_{ config.completion_callback }
  , send_canary_{ std::move(send_canary) }
{
}

void
lazy_circuit_breaker::reset()
{
    state_ = circuit_breaker_state::closed;
    total_ = 0;
    failed_ = 0;
    opened_at_ = std::chrono::nanoseconds::zero();
    window_start_ = std::chrono::steady_clock::now().time_since_epoch();
}

auto
lazy_circuit_breaker::state() -> circuit_breaker_state
{
    return state_;
}

auto
lazy_circuit_breaker::canary_timeout() -> std::chrono::nanoseconds
{
    return canary_timeout_;
}

auto
lazy_circuit_breaker::invoke_callback(std::error_code error) -> bool
{
    return completion_callback_(error);
}

auto
lazy_circuit_breaker::allows_request() -> bool
{
    if (state_ == circuit_breaker_state::closed) {
        return true;
    }

    auto elapsed = std::chrono::steady_clock::now().time_since_epoch() > (opened_at_.load() + sleep_window_);
    auto old_state{ circuit_breaker_state::open };
    if (elapsed && state_.compare_exchange_strong(old_state, circuit_breaker_state::half_open)) {
        // if we're outside the sleep window and the circuit is open then send a canary.
        send_canary_();
    }
    return false;
}

void
lazy_circuit_breaker::mark_successful()
{
    if (auto old_state{ circuit_breaker_state::half_open }; state_.compare_exchange_strong(old_state, circuit_breaker_state::closed)) {
        reset();
        LOG_DEBUG("moving circuit breaker closed");
        return;
    }
    maybe_reset_rolling_window();
    total_.fetch_add(1);
}

void
lazy_circuit_breaker::mark_failure()
{
    if (auto old_state{ circuit_breaker_state::half_open }; state_.compare_exchange_strong(old_state, circuit_breaker_state::open)) {
        opened_at_ = std::chrono::steady_clock::now().time_since_epoch();
        LOG_DEBUG("moving circuit breaker from half open to open");
        return;
    }

    maybe_reset_rolling_window();
    total_.fetch_add(1);
    failed_.fetch_add(1);
    maybe_open_circuit();
}

void
lazy_circuit_breaker::maybe_open_circuit()
{
    if (total_ < volume_threshold_) {
        return;
    }

    auto current_percentage = (static_cast<double>(failed_) / static_cast<double>(total_)) * 100.0;
    if (current_percentage >= error_percentage_threshold_) {
        state_ = circuit_breaker_state::open;
        opened_at_ = std::chrono::steady_clock::now().time_since_epoch();
        LOG_DEBUG("moving circuit breaker open");
    }
}

void
lazy_circuit_breaker::maybe_reset_rolling_window()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    if (window_start_.load() + rolling_window_ <= now) {
        return;
    }
    window_start_ = now;
    total_ = 0;
    failed_ = 0;
}
} // namespace couchbase::new_core
