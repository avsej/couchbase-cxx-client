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

#include <couchbase/codec/encoded_value.hxx>
#include <couchbase/error.hxx>
#include <couchbase/query_meta_data.hxx>
#include <couchbase/query_row.hxx>

#include <future>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>

namespace couchbase
{
#ifndef COUCHBASE_CXX_CLIENT_DOXYGEN
class internal_query_stream_result;
#endif

/**
 * The signature for the handler of the @ref query_stream_result#next() operation.
 *
 * @since 1.0.0
 * @volatile
 */
using query_row_handler = std::function<void(error, std::optional<query_row>)>;

/**
 * A streaming result handle for N1QL queries.
 *
 * Rows are fetched one at a time via @ref next(). The stream must be fully drained
 * (or @ref cancel() called) before @ref meta_data() resolves. Only a single @ref next()
 * call may be outstanding at a time.
 *
 * @since 1.0.0
 * @volatile
 */
class query_stream_result
{
public:
  /**
   * Constructs an empty (no-op) result handle.
   *
   * @since 1.0.0
   * @internal
   */
  query_stream_result() = default;

  /**
   * Constructs a query stream result from an internal result.
   *
   * @param internal the internal result handle
   *
   * @since 1.0.0
   * @internal
   */
  explicit query_stream_result(std::shared_ptr<internal_query_stream_result> internal);

  /**
   * Fetches the next row asynchronously, invoking the handler when ready.
   *
   * The handler receives ({}, row) for a real row, ({}, {}) at clean end-of-stream,
   * or (error, {}) if the stream ended with an error.
   *
   * Only one outstanding call is allowed at a time.
   *
   * @param handler callable that implements @ref query_row_handler
   *
   * @since 1.0.0
   * @volatile
   */
  void next(query_row_handler&& handler) const;

  /**
   * Fetches the next row, returning a future.
   *
   * Only one outstanding call is allowed at a time.
   *
   * @return future object that carries the result of the operation
   *
   * @since 1.0.0
   * @volatile
   */
  auto next() const -> std::future<std::pair<error, std::optional<query_row>>>;

  /**
   * Returns the query signature captured during preamble parsing, if present.
   *
   * @return optional binary JSON signature
   *
   * @since 1.0.0
   * @volatile
   */
  [[nodiscard]] auto signature() const -> std::optional<codec::binary>;

  /**
   * Returns the query metadata.
   *
   * The returned future resolves only after the stream has been fully drained
   * (all rows consumed or the stream cancelled). Intended to be called once.
   *
   * @return future carrying (error, query_meta_data)
   *
   * @since 1.0.0
   * @volatile
   */
  auto meta_data() const -> std::future<std::pair<error, query_meta_data>>;

  /**
   * Cancels the stream and closes the underlying HTTP connection.
   *
   * @since 1.0.0
   * @volatile
   */
  void cancel();

  /**
   * An input iterator that synchronously fetches rows one at a time.
   *
   * Iteration ends when an empty-row sentinel is encountered (end-of-stream or error).
   *
   * @since 1.0.0
   * @volatile
   */
  class iterator
  {
  public:
    auto operator==(const iterator& other) const -> bool;
    auto operator!=(const iterator& other) const -> bool;
    auto operator*() -> std::pair<error, query_row>;
    auto operator++() -> iterator&;

    explicit iterator(std::shared_ptr<internal_query_stream_result> internal);
    explicit iterator(std::pair<error, query_row> item);

    using difference_type = std::ptrdiff_t;
    using value_type = query_row;
    using pointer = const query_row*;
    using reference = const query_row&;
    using iterator_category = std::input_iterator_tag;

  private:
    void fetch_item();

    std::shared_ptr<internal_query_stream_result> internal_{};
    std::pair<error, query_row> item_{};
    bool terminal_{ false };
  };

  /**
   * Returns an iterator to the beginning.
   *
   * @return iterator to the first row
   *
   * @since 1.0.0
   * @volatile
   */
  auto begin() -> iterator;

  /**
   * Returns a sentinel iterator representing the end.
   *
   * @return end sentinel iterator
   *
   * @since 1.0.0
   * @volatile
   */
  auto end() -> iterator;

private:
  std::shared_ptr<internal_query_stream_result> internal_{};
};
} // namespace couchbase
