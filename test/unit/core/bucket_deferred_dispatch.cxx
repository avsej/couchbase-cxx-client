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

// A request parked in the bucket's deferred queue while the bucket bootstraps, and completed
// (by its deadline) before the bootstrap finishes, must not reach the server once the queue
// drains. The bucket bootstraps against a scripted loopback endpoint that withholds the cluster
// configuration until the case releases it, which holds the bucket unconfigured for as long as
// the case needs.

#include "framework/test_registry.hxx"

#include "framework/errors.hxx"

#include "unit/mcbp/mcbp_loopback.hxx"

#include "core/app_telemetry_meter.hxx"
#include "core/bucket.hxx"
#include "core/cluster_credentials.hxx"
#include "core/cluster_options.hxx"
#include "core/mcbp/queue_request.hxx"
#include "core/mcbp/queue_response.hxx"
#include "core/metrics/meter_wrapper.hxx"
#include "core/operations/document_get.hxx"
#include "core/origin.hxx"
#include "core/orphan_reporter.hxx"
#include "core/protocol/client_opcode.hxx"
#include "core/tls_context_provider.hxx"
#include "core/topology/configuration.hxx"
#include "core/tracing/tracer_wrapper.hxx"

#include <couchbase/error_codes.hxx>

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/post.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
#include <vector>

namespace couchbase::test
{
namespace
{
using namespace std::chrono_literals;
using couchbase::core::protocol::client_opcode;

constexpr auto bucket_name = "test-bucket";
constexpr auto wait_budget = 5s;

// A bucket bootstrapping against server, with the cluster configuration withheld, and an io thread
// running the bucket's context.
//
// The configuration request is the last step of the bootstrap. Once the server holds it, the
// bucket is unconfigured and stays so until release_cluster_config(); each case waits for that
// point with server_holds_the_configuration_request() before dispatching.
class bootstrapping_bucket
{
public:
  explicit bootstrapping_bucket(mcbp_loopback& server)
  {
    couchbase::core::cluster_credentials credentials{};
    credentials.username = "user";
    credentials.password = "pass";
    credentials.allowed_sasl_mechanisms = { { "PLAIN" } };
    bucket_ = std::make_shared<couchbase::core::bucket>(
      "test-client-id",
      io_,
      tls_,
      couchbase::core::tracing::tracer_wrapper::create(nullptr, nullptr),
      couchbase::core::metrics::meter_wrapper::create(nullptr, nullptr),
      std::make_shared<couchbase::core::orphan_reporter>(
        io_, couchbase::core::orphan_reporter_options{}),
      std::make_shared<couchbase::core::app_telemetry_meter>(),
      bucket_name,
      couchbase::core::origin{
        credentials, "127.0.0.1", server.port(), couchbase::core::cluster_options{} },
      std::vector<couchbase::core::protocol::hello_feature>{},
      nullptr);
    server.hold_cluster_config();
    io_thread_ = std::thread([this]() {
      io_.run();
    });
    run_on_io([this]() {
      bucket_->bootstrap([](std::error_code, couchbase::core::topology::configuration) {
      });
    });
  }

  bootstrapping_bucket(const bootstrapping_bucket&) = delete;
  bootstrapping_bucket(bootstrapping_bucket&&) = delete;
  auto operator=(const bootstrapping_bucket&) -> bootstrapping_bucket& = delete;
  auto operator=(bootstrapping_bucket&&) -> bootstrapping_bucket& = delete;

  ~bootstrapping_bucket()
  {
    run_on_io([this]() {
      bucket_->close();
    });
    io_.stop();
    io_thread_.join();
  }

  [[nodiscard]] auto get() const -> const std::shared_ptr<couchbase::core::bucket>&
  {
    return bucket_;
  }

  // Runs fn on the io thread and returns its result, so the case drives the bucket from the thread
  // its sessions run on, as production callers do. fn must not throw: an exception there escapes
  // io_context::run() on the io thread.
  template<typename Fn>
  auto run_on_io(Fn&& fn) -> std::invoke_result_t<Fn&>
  {
    using result_type = std::invoke_result_t<Fn&>;
    std::promise<result_type> done;
    asio::post(io_, [&fn, &done]() {
      if constexpr (std::is_void_v<result_type>) {
        fn();
        done.set_value();
      } else {
        done.set_value(fn());
      }
    });
    return done.get_future().get();
  }

private:
  asio::io_context io_{};
  asio::executor_work_guard<asio::io_context::executor_type> guard_{ io_.get_executor() };
  couchbase::core::tls_context_provider tls_{};
  std::shared_ptr<couchbase::core::bucket> bucket_{};
  std::thread io_thread_{};
};

auto
server_holds_the_configuration_request(const mcbp_loopback& server) -> bool
{
  return server.wait_until(
    [&server]() {
      return server.received(client_opcode::get_cluster_config) > 0;
    },
    wait_budget);
}

struct recorded_completion {
  std::atomic_int calls{ 0 };
  std::error_code ec{};
};

auto
make_get(const std::string& key, const std::shared_ptr<recorded_completion>& completion)
  -> std::shared_ptr<couchbase::core::mcbp::queue_request>
{
  auto req = std::make_shared<couchbase::core::mcbp::queue_request>(
    couchbase::core::protocol::magic::client_request,
    client_opcode::get,
    [completion](std::shared_ptr<couchbase::core::mcbp::queue_response>,
                 std::shared_ptr<couchbase::core::mcbp::queue_request>,
                 std::error_code ec) {
      completion->ec = ec;
      completion->calls.fetch_add(1);
    });
  req->key_ = std::vector<std::byte>(reinterpret_cast<const std::byte*>(key.data()),
                                     reinterpret_cast<const std::byte*>(key.data() + key.size()));
  return req;
}

// Releases the configuration, then waits for a request deferred after the cancelled one. The
// deferred queue drains in order onto one connection, so once the server has the sentinel it has
// everything the drain was going to send ahead of it.
void
drain_through(mcbp_loopback& server, bootstrapping_bucket& bucket, const std::string& sentinel)
{
  auto sentinel_completion = std::make_shared<recorded_completion>();
  assert_success(bucket.run_on_io([&]() {
    return bucket.get()->direct_dispatch(make_get(sentinel, sentinel_completion));
  }),
                 "the sentinel is deferred");
  server.release_cluster_config();
  assert_true(server.wait_until(
                [&]() {
                  return server.received_key(sentinel);
                },
                wait_budget),
              "the deferred queue drains to the server");
}

void
a_cancelled_direct_dispatch_is_not_sent_when_the_deferred_queue_drains(
  [[maybe_unused]] context& ctx)
{
  mcbp_loopback server{ bucket_name };
  bootstrapping_bucket bucket{ server };
  assert_true(server_holds_the_configuration_request(server), "the bucket is bootstrapping");
  assert_false(bucket.get()->is_configured(), "the configuration is withheld");

  const std::string cancelled_key{ "cancelled-while-deferred" };
  auto completion = std::make_shared<recorded_completion>();
  auto req = make_get(cancelled_key, completion);
  assert_success(bucket.run_on_io([&]() {
    auto ec = bucket.get()->direct_dispatch(req);
    // What crud_component's deadline timer does when it fires.
    req->cancel(couchbase::errc::common::unambiguous_timeout);
    return ec;
  }),
                 "the request is deferred");
  assert_eq(completion->calls.load(), 1, "the deadline completes the request");

  drain_through(server, bucket, "sentinel");

  assert_false(server.received_key(cancelled_key),
               "a request completed while deferred is not sent to the server");
  assert_eq(completion->calls.load(), 1, "the request completes exactly once");
  assert_eq(completion->ec,
            std::error_code{ couchbase::errc::common::unambiguous_timeout },
            "with the deadline's error");
}

void
a_timed_out_classic_command_is_not_sent_when_the_deferred_queue_drains(
  [[maybe_unused]] context& ctx)
{
  // Control for the case above, through bucket::execute(): mcbp_command::send_to() drops a
  // command whose handler has already run.
  mcbp_loopback server{ bucket_name };
  bootstrapping_bucket bucket{ server };
  assert_true(server_holds_the_configuration_request(server), "the bucket is bootstrapping");
  assert_false(bucket.get()->is_configured(), "the configuration is withheld");

  const std::string timed_out_key{ "timed-out-while-deferred" };
  std::promise<std::error_code> timed_out;
  bucket.run_on_io([&]() {
    couchbase::core::operations::get_request request{
      couchbase::core::document_id{ bucket_name, "_default", "_default", timed_out_key },
    };
    request.timeout = 1ms;
    bucket.get()->execute(request, [&timed_out](couchbase::core::operations::get_response&& resp) {
      timed_out.set_value(resp.ctx.ec());
    });
  });
  assert_eq(timed_out.get_future().get(),
            std::error_code{ couchbase::errc::common::unambiguous_timeout },
            "the deadline completes the command");

  drain_through(server, bucket, "sentinel");

  assert_false(server.received_key(timed_out_key),
               "a command completed while deferred is not sent to the server");
}
} // namespace

auto
tests() -> test_suite
{
  return {
    suite_name,
    {
      { CASE(a_cancelled_direct_dispatch_is_not_sent_when_the_deferred_queue_drains),
        {},
        timeout::network },
      { CASE(a_timed_out_classic_command_is_not_sent_when_the_deferred_queue_drains),
        {},
        timeout::network },
    },
  };
}

} // namespace couchbase::test
