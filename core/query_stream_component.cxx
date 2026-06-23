/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2024. Couchbase, Inc.
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

#include "query_stream_component.hxx"

#include "free_form_http_request.hxx"
#include "http_component.hxx"
#include "impl/bootstrap_error.hxx"
#include "logger/logger.hxx"
#include "platform/uuid.h"
#include "service_type.hxx"
#include "utils/json.hxx"

#include <couchbase/error_codes.hxx>

#include <asio/io_context.hpp>
#include <gsl/assert>
#include <spdlog/fmt/bundled/core.h>
#include <tao/json/value.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace couchbase::core
{
namespace
{
#ifdef COUCHBASE_CXX_CLIENT_COLUMNAR
auto
to_error_code(const error_union& err) -> std::error_code
{
  if (std::holds_alternative<std::error_code>(err)) {
    return std::get<std::error_code>(err);
  }
  if (std::holds_alternative<impl::bootstrap_error>(err)) {
    return std::get<impl::bootstrap_error>(err).ec;
  }
  return {};
}
#else
auto
to_error_code(std::error_code err) -> std::error_code
{
  return err;
}
#endif

auto
build_streaming_query_body(const operations::query_request& request,
                           const std::string& client_context_id,
                           std::chrono::milliseconds timeout) -> std::string
{
  tao::json::value body{
    { "client_context_id", client_context_id },
    { "statement", request.statement },
  };

  auto timeout_for_service = timeout;
  if (timeout_for_service > std::chrono::milliseconds(5'000)) {
    // Tell the query engine its deadline is 500ms tighter than ours, so we always observe the
    // server-side timeout response before our own deadline fires.
    timeout_for_service -= std::chrono::milliseconds(500);
  }
  body["timeout"] = fmt::format("{}ms", timeout_for_service.count());

  for (const auto& [name, value] : request.named_parameters) {
    Expects(!name.empty());
    std::string key = name;
    if (key[0] != '$') {
      key.insert(key.begin(), '$');
    }
    body[key] = utils::json::parse(value);
  }
  if (!request.positional_parameters.empty()) {
    std::vector<tao::json::value> parameters;
    parameters.reserve(request.positional_parameters.size());
    for (const auto& value : request.positional_parameters) {
      parameters.emplace_back(utils::json::parse(value));
    }
    body["args"] = std::move(parameters);
  }
  if (request.profile.has_value()) {
    switch (request.profile.value()) {
      case query_profile::phases:
        body["profile"] = "phases";
        break;
      case query_profile::timings:
        body["profile"] = "timings";
        break;
      case query_profile::off:
        body["profile"] = "off";
        break;
    }
  }
  if (request.use_replica.has_value()) {
    // The capability guard (errc::common::feature_not_available when the cluster does not support
    // read-from-replica) is enforced before dispatch in cluster::query_stream, where the live
    // configuration capabilities are reachable; here we only emit the encoded field.
    body["use_replica"] = request.use_replica.value() ? "on" : "off";
  }
  if (request.max_parallelism.has_value()) {
    body["max_parallelism"] = std::to_string(request.max_parallelism.value());
  }
  if (request.pipeline_cap.has_value()) {
    body["pipeline_cap"] = std::to_string(request.pipeline_cap.value());
  }
  if (request.pipeline_batch.has_value()) {
    body["pipeline_batch"] = std::to_string(request.pipeline_batch.value());
  }
  if (request.scan_cap.has_value()) {
    body["scan_cap"] = std::to_string(request.scan_cap.value());
  }
  if (!request.metrics) {
    body["metrics"] = false;
  }
  if (request.readonly) {
    body["readonly"] = true;
  }
  if (request.flex_index) {
    body["use_fts"] = true;
  }
  if (request.preserve_expiry) {
    body["preserve_expiry"] = true;
  }
  bool check_scan_wait = false;
  if (request.scan_consistency.has_value()) {
    switch (request.scan_consistency.value()) {
      case query_scan_consistency::not_bounded:
        body["scan_consistency"] = "not_bounded";
        break;
      case query_scan_consistency::request_plus:
        check_scan_wait = true;
        body["scan_consistency"] = "request_plus";
        break;
    }
  } else if (!request.mutation_state.empty()) {
    check_scan_wait = true;
    body["scan_consistency"] = "at_plus";
    tao::json::value scan_vectors = tao::json::empty_object;
    for (const auto& token : request.mutation_state) {
      auto* bucket = scan_vectors.find(token.bucket_name());
      if (bucket == nullptr) {
        scan_vectors[token.bucket_name()] = tao::json::empty_object;
        bucket = scan_vectors.find(token.bucket_name());
      }
      auto& bucket_obj = bucket->get_object();
      bucket_obj[std::to_string(token.partition_id())] =
        std::vector<tao::json::value>{ token.sequence_number(),
                                       std::to_string(token.partition_uuid()) };
    }
    body["scan_vectors"] = scan_vectors;
  }
  if (check_scan_wait && request.scan_wait.has_value()) {
    body["scan_wait"] = fmt::format("{}ms", request.scan_wait.value().count());
  }
  if (request.query_context.has_value()) {
    body["query_context"] = request.query_context.value();
  }
  for (const auto& [name, value] : request.raw) {
    body[name] = utils::json::parse(value);
  }

  return utils::json::generate(body);
}
} // namespace

class query_stream_component_impl : public std::enable_shared_from_this<query_stream_component_impl>
{
public:
  query_stream_component_impl(asio::io_context& io,
                              http_component http,
                              std::chrono::milliseconds default_timeout)
    : io_{ io }
    , http_{ std::move(http) }
    , default_timeout_{ default_timeout }
  {
  }

  void execute(operations::query_request request, query_stream_component::handler_type&& handler)
  {
    const std::string client_context_id =
      request.client_context_id.value_or(uuid::to_string(uuid::random()));
    const std::chrono::milliseconds timeout = request.timeout.value_or(default_timeout_);

    http_request http_req{ service_type::query, "POST" };
    http_req.path = "/query/service";
    http_req.body = build_streaming_query_body(request, client_context_id, timeout);
    http_req.timeout = timeout;
    http_req.client_context_id = client_context_id;
    http_req.is_read_only = request.readonly;
    http_req.headers["connection"] = "keep-alive";
    http_req.headers["content-type"] = "application/json";
    if (request.parent_span) {
      http_req.parent_span = request.parent_span;
    }

    auto callback_state = std::make_shared<callback_state_type>(std::move(handler));

#ifdef COUCHBASE_CXX_CLIENT_COLUMNAR
    using dispatch_error = error_union;
#else
    using dispatch_error = std::error_code;
#endif

    auto op = http_.do_http_request(
      http_req,
      [self = shared_from_this(), callback_state](http_response resp, dispatch_error err) mutable {
        if (auto ec = to_error_code(err)) {
          self->invoke(callback_state, {}, ec);
          return;
        }
        auto stream = std::make_shared<query_stream>(self->io_, resp.body());
        stream->start([self, callback_state, stream](std::error_code early_error) mutable {
          if (early_error) {
            self->invoke(callback_state, {}, early_error);
            return;
          }
          self->invoke(callback_state, std::move(*stream), {});
        });
      });

    if (!op.has_value()) {
      invoke(callback_state, {}, to_error_code(op.error()));
    }
  }

private:
  struct callback_state_type {
    explicit callback_state_type(query_stream_component::handler_type h)
      : handler{ std::move(h) }
    {
    }

    query_stream_component::handler_type handler;
    std::mutex mutex{};
    bool invoked{ false };
  };

  static void invoke(const std::shared_ptr<callback_state_type>& state,
                     query_stream stream,
                     std::error_code ec)
  {
    query_stream_component::handler_type handler;
    {
      const std::scoped_lock lock{ state->mutex };
      if (state->invoked) {
        return;
      }
      state->invoked = true;
      handler = std::move(state->handler);
    }
    if (handler) {
      handler(std::move(stream), ec);
    }
  }

  asio::io_context& io_;
  http_component http_;
  std::chrono::milliseconds default_timeout_;
};

query_stream_component::query_stream_component(asio::io_context& io,
                                               http_component http,
                                               std::chrono::milliseconds default_timeout)
  : impl_{ std::make_shared<query_stream_component_impl>(io, std::move(http), default_timeout) }
{
}

void
query_stream_component::execute(operations::query_request request, handler_type&& handler) const
{
  impl_->execute(std::move(request), std::move(handler));
}
} // namespace couchbase::core
