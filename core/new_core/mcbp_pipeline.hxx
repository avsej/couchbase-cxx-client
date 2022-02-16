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

#include "mcbp_operation_queue.hxx"
#include "mcbp_pipeline_client.hxx"
#include "mcbp_queue_request.hxx"
#include "route_endpoint.hxx"

#include <cstddef>
#include <string>
#include <system_error>

namespace couchbase::new_core
{
class mcbp_pipeline : public std::enable_shared_from_this<mcbp_pipeline>
{
  public:
    mcbp_pipeline(const route_endpoint& endpoint, std::size_t max_clients, std::size_t max_items);

    [[nodiscard]] auto address() const -> const std::string&;
    [[nodiscard]] auto host() const -> std::string;
    [[nodiscard]] auto is_seed_node() const -> bool;
    [[nodiscard]] auto requeue_request(std::shared_ptr<mcbp_queue_request> request) -> std::error_code;
    [[nodiscard]] auto send_request(std::shared_ptr<mcbp_queue_request> request) -> std::error_code;

    void start_clients();

    [[nodiscard]] auto debug_string() const -> std::string;

  private:
    [[nodiscard]] auto send_request(std::shared_ptr<mcbp_queue_request> request, std::size_t max_items) -> std::error_code;

    std::string address_;
    std::size_t max_clients_;
    std::size_t max_items_;
    bool is_seed_node_;
    std::vector<std::shared_ptr<mcbp_pipeline_client>> clients_{};
    std::mutex clients_mutex_{};
    mcbp_operation_queue queue_{};
};

auto
make_mcbp_dead_pipeline(std::size_t max_items) -> std::shared_ptr<mcbp_pipeline>;
} // namespace couchbase::new_core
