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

#include <couchbase/error_codes.hxx>
#include <couchbase/query_meta_data.hxx>
#include <couchbase/query_metrics.hxx>
#include <couchbase/query_status.hxx>
#include <couchbase/query_stream_result.hxx>
#include <couchbase/query_warning.hxx>

#include "core/operations/document_query.hxx"
#include "core/utils/binary.hxx"

#include "internal_query_stream_result.hxx"

#include <algorithm>
#include <cctype>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace couchbase
{
namespace
{
auto
to_binary(const std::string& s) -> codec::binary
{
  return core::utils::to_binary(s);
}

auto
map_query_status(std::string status) -> query_status
{
  std::transform(status.cbegin(), status.cend(), status.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (status == "running") {
    return query_status::running;
  }
  if (status == "success") {
    return query_status::success;
  }
  if (status == "errors") {
    return query_status::errors;
  }
  if (status == "completed") {
    return query_status::completed;
  }
  if (status == "stopped") {
    return query_status::stopped;
  }
  if (status == "timeout") {
    return query_status::timeout;
  }
  if (status == "closed") {
    return query_status::closed;
  }
  if (status == "fatal") {
    return query_status::fatal;
  }
  if (status == "aborted") {
    return query_status::aborted;
  }
  return query_status::unknown;
}

auto
build_query_meta_data(core::operations::query_response::query_meta_data meta) -> query_meta_data
{
  std::vector<query_warning> warnings;
  if (meta.warnings) {
    warnings.reserve(meta.warnings->size());
    for (auto& w : *meta.warnings) {
      warnings.emplace_back(w.code, std::move(w.message), w.reason, w.retry);
    }
  }

  std::optional<query_metrics> metrics;
  if (meta.metrics) {
    metrics = query_metrics{
      meta.metrics->elapsed_time, meta.metrics->execution_time, meta.metrics->result_count,
      meta.metrics->result_size,  meta.metrics->sort_count,     meta.metrics->mutation_count,
      meta.metrics->error_count,  meta.metrics->warning_count,
    };
  }

  std::optional<codec::binary> signature;
  if (meta.signature) {
    signature = to_binary(*meta.signature);
  }

  std::optional<codec::binary> profile;
  if (meta.profile) {
    profile = to_binary(*meta.profile);
  }

  return query_meta_data{
    std::move(meta.request_id),
    std::move(meta.client_context_id),
    map_query_status(std::move(meta.status)),
    std::move(warnings),
    metrics,
    std::move(signature),
    std::move(profile),
  };
}
} // namespace

// ---------------------------------------------------------------------------
// internal_query_stream_result implementation
// ---------------------------------------------------------------------------

internal_query_stream_result::internal_query_stream_result(core::query_stream stream)
  : stream_{ std::move(stream) }
{
}

internal_query_stream_result::~internal_query_stream_result()
{
  cancel();
}

void
internal_query_stream_result::cancel()
{
  stream_.cancel();
}

auto
internal_query_stream_result::signature() const -> std::optional<codec::binary>
{
  auto raw = stream_.signature();
  if (!raw) {
    return {};
  }
  return to_binary(*raw);
}

void
internal_query_stream_result::resolve_meta_data(std::error_code ec)
{
  std::pair<error, query_meta_data> value;
  if (ec) {
    value.first = error(ec, "query stream ended with an error");
  }
  auto core_meta = stream_.meta_data();
  if (core_meta) {
    value.second = build_query_meta_data(std::move(*core_meta));
  }

  std::shared_ptr<std::promise<std::pair<error, query_meta_data>>> promise;
  {
    const std::scoped_lock lk{ meta_mutex_ };
    if (!terminal_reached_) {
      terminal_reached_ = true;
      terminal_value_ = value;
      promise = meta_promise_;
    }
  }
  if (promise) {
    promise->set_value(std::move(value));
  }
}

auto
internal_query_stream_result::meta_data() -> std::future<std::pair<error, query_meta_data>>
{
  const std::scoped_lock lk{ meta_mutex_ };
  if (terminal_reached_) {
    std::promise<std::pair<error, query_meta_data>> p;
    p.set_value(terminal_value_);
    return p.get_future();
  }
  return meta_promise_->get_future();
}

void
internal_query_stream_result::next(query_row_handler&& handler)
{
  stream_.next_row([handler = std::move(handler), self = this](std::optional<std::string> row,
                                                               std::error_code ec) mutable {
    if (row.has_value()) {
      // Real row — ec is falsy
      return handler({}, query_row{ to_binary(*row) });
    }
    // Terminal: nullopt row
    self->resolve_meta_data(ec);
    if (ec) {
      return handler(error(ec, "query stream ended with an error"), {});
    }
    return handler({}, {});
  });
}

// ---------------------------------------------------------------------------
// query_stream_result public implementation
// ---------------------------------------------------------------------------

query_stream_result::query_stream_result(std::shared_ptr<internal_query_stream_result> internal)
  : internal_{ std::move(internal) }
{
}

void
query_stream_result::next(query_row_handler&& handler) const
{
  if (!internal_) {
    // Empty handle behaves as an immediately-exhausted stream.
    handler(error{}, std::nullopt);
    return;
  }
  return internal_->next(std::move(handler));
}

auto
query_stream_result::next() const -> std::future<std::pair<error, std::optional<query_row>>>
{
  auto barrier = std::make_shared<std::promise<std::pair<error, std::optional<query_row>>>>();
  if (!internal_) {
    // Empty handle behaves as an immediately-exhausted stream.
    barrier->set_value({ error{}, std::nullopt });
    return barrier->get_future();
  }
  internal_->next([barrier](const auto& err, const auto& row) mutable {
    barrier->set_value({ err, row });
  });
  return barrier->get_future();
}

auto
query_stream_result::signature() const -> std::optional<codec::binary>
{
  if (!internal_) {
    return {};
  }
  return internal_->signature();
}

auto
query_stream_result::meta_data() const -> std::future<std::pair<error, query_meta_data>>
{
  if (!internal_) {
    // Empty handle resolves immediately with empty metadata.
    std::promise<std::pair<error, query_meta_data>> p;
    p.set_value({ error{}, query_meta_data{} });
    return p.get_future();
  }
  return internal_->meta_data();
}

void
query_stream_result::cancel()
{
  if (internal_) {
    return internal_->cancel();
  }
}

// ---------------------------------------------------------------------------
// iterator implementation
// ---------------------------------------------------------------------------

query_stream_result::iterator::iterator(std::shared_ptr<internal_query_stream_result> internal)
  : internal_{ std::move(internal) }
{
  fetch_item();
}

query_stream_result::iterator::iterator(std::pair<error, query_row> item)
  : item_{ std::move(item) }
  , terminal_{ true }
{
}

void
query_stream_result::iterator::fetch_item()
{
  auto barrier = std::make_shared<std::promise<std::pair<error, std::optional<query_row>>>>();
  internal_->next([barrier](const error& err, std::optional<query_row> row) mutable {
    barrier->set_value({ err, std::move(row) });
  });
  auto result = barrier->get_future().get();
  if (result.first) {
    // Error — store it and mark terminal so the iterator compares equal to end()
    item_ = { result.first, {} };
    terminal_ = true;
  } else if (!result.second.has_value()) {
    // Clean end-of-stream
    item_ = {};
    terminal_ = true;
  } else {
    item_ = { {}, *result.second };
    terminal_ = false;
  }
}

auto
query_stream_result::iterator::operator*() -> std::pair<error, query_row>
{
  return item_;
}

auto
query_stream_result::iterator::operator++() -> query_stream_result::iterator&
{
  fetch_item();
  return *this;
}

auto
query_stream_result::iterator::operator==(const query_stream_result::iterator& other) const -> bool
{
  // Compares only the terminal_ flags: this iterator is @volatile input-only and is
  // intended to be compared against end() (the range-for idiom), not for general equality.
  return terminal_ == other.terminal_;
}

auto
query_stream_result::iterator::operator!=(const query_stream_result::iterator& other) const -> bool
{
  return !(*this == other);
}

auto
query_stream_result::begin() -> query_stream_result::iterator
{
  if (!internal_) {
    // Empty handle has no rows; begin() == end().
    return end();
  }
  return query_stream_result::iterator(internal_);
}

auto
query_stream_result::end() -> query_stream_result::iterator
{
  return query_stream_result::iterator(std::pair<error, query_row>{ {}, {} });
}
} // namespace couchbase
