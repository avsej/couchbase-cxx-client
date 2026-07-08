/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2026-Present Couchbase, Inc.
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

#include "wait_until_ready.hxx"

#include "core/diagnostics.hxx"
#include "core/topology/configuration.hxx"
#include "with_bucket_config_or_timeout.hxx"

#include <couchbase/error.hxx>
#include <couchbase/error_codes.hxx>

#include <asio/steady_timer.hpp>

#include <algorithm>
#include <memory>
#include <utility>

namespace couchbase::core::impl
{
namespace
{
// The SDK refreshes the cluster map on its own interval (2.5s by default), so a freshly created
// bucket's vbucket map will not change faster than that regardless; a short poll just keeps latency
// low once it does.
constexpr std::chrono::milliseconds poll_interval{ 100 };

// online   : every requested service has at least one endpoint and ALL of them are ok.
// degraded : every requested service has at least one endpoint that is ok.
// When no services were requested we consider whatever the report contains; an empty report
// (nothing pinged back yet) is treated as not-ready.
auto
ping_predicate_satisfied(const diag::ping_result& report,
                         couchbase::cluster_state state,
                         const std::set<service_type>& requested) -> bool
{
  const auto& endpoints = report.services;

  std::set<service_type> services = requested;
  if (services.empty()) {
    for (const auto& [service, reports] : endpoints) {
      services.insert(service);
    }
    if (services.empty()) {
      return false;
    }
  }

  for (const auto service : services) {
    const auto it = endpoints.find(service);
    if (it == endpoints.end() || it->second.empty()) {
      return false;
    }
    const auto& reports = it->second;
    const auto is_ok = [](const diag::endpoint_ping_info& r) {
      return r.state == diag::ping_state::ok;
    };
    const bool ok = (state == couchbase::cluster_state::online)
                      ? std::all_of(reports.begin(), reports.end(), is_ok)
                      : std::any_of(reports.begin(), reports.end(), is_ok);
    if (!ok) {
      return false;
    }
  }
  return true;
}

// True once every vbucket has its active and all replica copies assigned to a node. A freshly
// created bucket reports an empty/partial vbucket map (replica slots set to -1) until the server
// finishes placing replicas; durable (MAJORITY) writes are ambiguous until then.
auto
vbucket_map_ready(const topology::configuration& config) -> bool
{
  if (!config.vbmap.has_value()) {
    return false;
  }
  const auto& vbmap = config.vbmap.value();
  if (vbmap.empty()) {
    return false;
  }
  // active + replicas
  const auto copies = static_cast<std::size_t>(config.num_replicas.value_or(0)) + 1;
  for (const auto& chain : vbmap) {
    if (chain.size() < copies) {
      return false;
    }
    for (std::size_t i = 0; i < copies; ++i) {
      if (chain[i] < 0) { // -1 => copy not yet assigned to a node
        return false;
      }
    }
  }
  return true;
}

// Self-owning async poll loop. Exactly one continuation (config fetch, ping, or the poll timer) is
// in flight at a time, each holding a shared_ptr to keep the operation alive; every continuation
// returns to poll()/schedule_retry(), which re-check the deadline, so the whole operation is
// bounded by `timeout` without a separate deadline timer.
class wait_until_ready_operation : public std::enable_shared_from_this<wait_until_ready_operation>
{
public:
  wait_until_ready_operation(core::cluster core,
                             std::optional<std::string> bucket_name,
                             std::chrono::milliseconds timeout,
                             couchbase::cluster_state desired_state,
                             std::set<service_type> services,
                             utils::movable_function<void(std::error_code)> handler)
    : core_{ std::move(core) }
    , bucket_name_{ std::move(bucket_name) }
    , deadline_{ std::chrono::steady_clock::now() + timeout }
    , desired_state_{ desired_state }
    , services_{ std::move(services) }
    , handler_{ std::move(handler) }
    , timer_{ core_.io_context() }
  {
  }

  void run()
  {
    if (desired_state_ == couchbase::cluster_state::offline) {
      return complete(errc::common::invalid_argument);
    }
    if (bucket_name_) {
      // Opening the bucket is what makes its configuration (and therefore the vbucket map we poll)
      // available on the core cluster. It is idempotent for an already-open bucket.
      const auto& bucket_name = *bucket_name_;
      auto self = shared_from_this();
      core_.open_bucket(bucket_name, [self](std::error_code /* ec */) {
        self->poll();
      });
    } else {
      poll();
    }
  }

private:
  auto remaining() const -> std::chrono::milliseconds
  {
    const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline_ - std::chrono::steady_clock::now());
    return std::max(left, std::chrono::milliseconds::zero());
  }

  void complete(std::error_code ec)
  {
    if (completed_) {
      return;
    }
    completed_ = true;
    handler_(ec);
  }

  void schedule_retry()
  {
    if (std::chrono::steady_clock::now() >= deadline_) {
      return complete(errc::common::unambiguous_timeout);
    }
    timer_.expires_after(poll_interval);
    auto self = shared_from_this();
    timer_.async_wait([self](std::error_code timer_ec) {
      if (timer_ec) {
        return; // cancelled / io_context shutting down
      }
      self->poll();
    });
  }

  void poll()
  {
    if (completed_) {
      return;
    }
    if (remaining() == std::chrono::milliseconds::zero()) {
      return complete(errc::common::unambiguous_timeout);
    }
    if (bucket_name_) {
      // KV readiness (replica placement) first -- it is the condition durable writes need and the
      // one a fresh bucket fails; then confirm service health with a ping.
      const auto& bucket_name = *bucket_name_;
      auto self = shared_from_this();
      with_bucket_config_or_timeout<bool>(
        core_,
        bucket_name,
        remaining(),
        [self](couchbase::error err, bool ready) {
          if (err.ec() || !ready) {
            return self->schedule_retry();
          }
          self->do_ping();
        },
        [](const std::shared_ptr<topology::configuration>& config)
          -> std::pair<std::error_code, bool> {
          return { {}, config && vbucket_map_ready(*config) };
        });
    } else {
      do_ping();
    }
  }

  void do_ping()
  {
    auto self = shared_from_this();
    core_.ping(
      std::nullopt, bucket_name_, services_, remaining(), [self](const diag::ping_result& report) {
        if (ping_predicate_satisfied(report, self->desired_state_, self->services_)) {
          return self->complete({});
        }
        self->schedule_retry();
      });
  }

  core::cluster core_;
  std::optional<std::string> bucket_name_;
  std::chrono::steady_clock::time_point deadline_;
  couchbase::cluster_state desired_state_;
  std::set<service_type> services_;
  utils::movable_function<void(std::error_code)> handler_;
  asio::steady_timer timer_;
  bool completed_{ false };
};
} // namespace

void
wait_until_ready(core::cluster core,
                 std::optional<std::string> bucket_name,
                 std::chrono::milliseconds timeout,
                 couchbase::cluster_state desired_state,
                 std::set<service_type> services,
                 utils::movable_function<void(std::error_code)> handler)
{
  auto op = std::make_shared<wait_until_ready_operation>(std::move(core),
                                                         std::move(bucket_name),
                                                         timeout,
                                                         desired_state,
                                                         std::move(services),
                                                         std::move(handler));
  op->run();
}
} // namespace couchbase::core::impl
