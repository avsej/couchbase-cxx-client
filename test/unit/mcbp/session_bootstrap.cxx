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

// mcbp_session bootstrap against a scripted loopback endpoint (unit/mcbp/mcbp_loopback.hxx).

#include "framework/test_registry.hxx"

#include "unit/mcbp/mcbp_loopback.hxx"

#include "core/cluster_credentials.hxx"
#include "core/cluster_options.hxx"
#include "core/io/mcbp_session.hxx"
#include "core/origin.hxx"
#include "core/topology/configuration.hxx"

#include <couchbase/retry_reason.hxx>

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/post.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <system_error>
#include <thread>

namespace couchbase::test
{
namespace
{
using namespace std::chrono_literals;

// Bootstraps one session against server and returns the bootstrap result. The session is stopped
// and its io thread joined before returning.
auto
bootstrap_once(const mcbp_loopback& server, couchbase::core::cluster_options options)
  -> std::error_code
{
  asio::io_context io;
  auto guard = asio::make_work_guard(io);
  std::thread io_thread([&io]() {
    io.run();
  });

  couchbase::core::cluster_credentials credentials{};
  credentials.username = "user";
  credentials.password = "pass";
  credentials.allowed_sasl_mechanisms = { { "PLAIN" } };
  couchbase::core::io::mcbp_session session{
    "test-client-id",
    "test-node-uuid",
    io,
    couchbase::core::origin{ credentials, "127.0.0.1", server.port(), std::move(options) },
    nullptr,
  };

  std::promise<std::error_code> result;
  asio::post(io, [&session, &result]() {
    session.bootstrap([&result](std::error_code ec, couchbase::core::topology::configuration) {
      result.set_value(ec);
    });
  });
  auto ec = result.get_future().get();

  session.stop(couchbase::retry_reason::do_not_retry);
  guard.reset();
  io_thread.join();
  return ec;
}

void
the_loopback_endpoint_bootstraps_a_session([[maybe_unused]] context& ctx)
{
  const mcbp_loopback server;
  couchbase::core::cluster_options options{};
  options.bootstrap_timeout = 3s;

  assert_eq(bootstrap_once(server, options), std::error_code{}, "the session bootstraps");
  assert_eq(server.accepted(), std::size_t{ 1 }, "one connection, never torn down");
}

void
a_connection_deadline_queued_behind_its_connect_does_not_tear_the_connection_down(
  [[maybe_unused]] context& ctx)
{
  // A zero connect_timeout leaves the connect deadline expired when do_connect() arms it, and a
  // loopback connect has completed by the next reactor poll. Both completions are then ready in
  // one poll, and asio queues the socket's ahead of the timer's, so on_connect() runs first and
  // its connection_deadline_.cancel() has nothing left to retract. The deadline handler that runs
  // next must see that its connect phase is over and leave the established connection alone.
  const mcbp_loopback server;
  couchbase::core::cluster_options options{};
  options.connect_timeout = 0ms;
  options.bootstrap_timeout = 3s;

  assert_eq(bootstrap_once(server, options),
            std::error_code{},
            "a connection that completed before its deadline handler ran is kept");
}
} // namespace

auto
tests() -> test_suite
{
  return {
    suite_name,
    {
      { CASE(the_loopback_endpoint_bootstraps_a_session), {}, timeout::network },
      { CASE(a_connection_deadline_queued_behind_its_connect_does_not_tear_the_connection_down),
        {},
        timeout::network },
    },
  };
}

} // namespace couchbase::test
