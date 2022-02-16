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

#include "impl/fwd/error_map_component.hxx"

#include "mcbp/packet.hxx"
#include "mcbp_queue_callback.hxx"
#include "mcbp_queue_request_connection_info.hxx"
#include "pending_operation.hxx"
#include "retry_strategy.hxx"

#include <memory>
#include <mutex>
#include <set>

namespace couchbase::new_core
{
class mcbp_queue_request_builder;
class mcbp_operation_queue;

class mcbp_queue_request
  : public mcbp::packet
  , public retry_request
  , public pending_operation
{
  public:
    mcbp_queue_request(mcbp::command_magic magic, mcbp::command_code opcode, mcbp_queue_callback callback);
    ~mcbp_queue_request() override = default;

    [[nodiscard]] auto retry_attempts() const -> std::size_t override;
    [[nodiscard]] auto identifier() const -> std::string override;
    [[nodiscard]] auto idempotent() const -> bool override;
    [[nodiscard]] auto retry_reasons() const -> std::set<retry_reason> override;

    [[nodiscard]] auto retries() const -> std::pair<std::size_t, std::set<retry_reason>>;
    [[nodiscard]] auto connection_info() const -> mcbp_queue_request_connection_info;

    void record_retry_attempt(retry_reason reason) override;

    [[nodiscard]] auto is_cancelled() const -> bool;
    void cancel() override;

  private:
    // Static routing properties
    mcbp_queue_callback callback_;
    std::size_t replica_index_{ 0 };
    bool persistent_{ false };

    // This tracks when the request was dispatched so that we can properly prioritize older requests to try and meet timeout requirements.
    std::chrono::steady_clock::time_point dispatched_time_{};

    // This stores a pointer to the server that currently own this request.  This allows us to remove it from that list whenever the request
    // is cancelled.
    std::atomic<mcbp_operation_queue*> queued_with_{ nullptr };

    // This keeps track of whether the request has been 'completed' which is synonymous with the callback having been invoked. This is an
    // integer to allow us to atomically control it.
    std::atomic_bool is_completed_{ false };

    // This is used to lock access to the request when processing a timeout, a response or spans
    mutable std::mutex processing_mutex_{};

    // This stores the number of times that the item has been retried. It is used for various non-linear retry algorithms.
    std::size_t retry_count_{ 0 };

    // This is used to determine what, if any, retry strategy to use when deciding whether to retry the request and calculating any back-off
    // time period.
    std::shared_ptr<retry_strategy> retry_strategy_{};

    // This is the set of reasons why this request has been retried.
    std::set<retry_reason> retry_reasons_{};

    // This is used to lock access to the request when processing retry reasons or attempts.
    mutable std::mutex retry_mutex_{};

    mcbp_queue_request_connection_info connection_info_{};
    mutable std::mutex connection_info_mutex_{};

    std::string collection_name_{};
    std::string scope_name_{};

    friend mcbp_queue_request_builder;
    friend mcbp_operation_queue;
    friend impl::error_map_component;
};
} // namespace couchbase::new_core
