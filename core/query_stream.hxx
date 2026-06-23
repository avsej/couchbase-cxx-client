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

#pragma once

#include "core/operations/document_query.hxx"
#include "row_streamer.hxx"
#include "utils/movable_function.hxx"

#include <memory>
#include <optional>
#include <string>
#include <system_error>

namespace asio
{
class io_context;
} // namespace asio

namespace couchbase::core
{
class query_stream_impl;
class http_response_body;

/**
 * L3 streaming state machine for N1QL query responses.
 *
 * Wraps a row_streamer (pointed at "/results/^") and adds query-specific semantics:
 * preamble parsing (signature + upfront error detection), late trailer metadata, and
 * terminal error classification via map_query_error.
 */
class query_stream
{
public:
  query_stream(asio::io_context& io, http_response_body body, row_streamer_options options = {});

  /**
   * Starts the stream. Resolves once the preamble has been parsed. The early_error is set when
   * the response carried an upfront error (or the preamble failed to parse).
   */
  void start(utils::movable_function<void(std::error_code early_error)>&& on_ready);

  /**
   * Retrieves the next row. A populated row is delivered with a falsy error_code. An empty row
   * (std::nullopt) signals the end: a falsy error_code means a clean success end, a truthy
   * error_code means a trailing query/transport error.
   */
  void next_row(
    utils::movable_function<void(std::optional<std::string> row, std::error_code)>&& handler);

  /**
   * Signature captured from the preamble (available after start resolves).
   */
  [[nodiscard]] auto signature() const -> std::optional<std::string>;

  /**
   * Trailer metadata. Valid only after the stream has reached its end.
   */
  [[nodiscard]] auto meta_data() const
    -> std::optional<operations::query_response::query_meta_data>;

  /**
   * Cancels the stream & closes the HTTP connection.
   */
  void cancel();

private:
  std::shared_ptr<query_stream_impl> impl_;
};
} // namespace couchbase::core
