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

#include "mcbp_pipeline.hxx"
#include "errors.hxx"
#include "impl/address_utils.hxx"

#include <fmt/core.h>

#include <vector>

namespace couchbase::new_core
{
mcbp_pipeline::mcbp_pipeline(const route_endpoint& endpoint, std::size_t max_clients, std::size_t max_items)
  : address_{ endpoint.address }
  , max_clients_{ max_clients }
  , max_items_{ max_items }
  , is_seed_node_{ endpoint.is_seed_node }
{
}

auto
mcbp_pipeline::address() const -> const std::string&
{
    return address_;
}

auto
mcbp_pipeline::host() const -> std::string
{
    return utils::host_from_host_port(address_);
}

auto
mcbp_pipeline::is_seed_node() const -> bool
{
    return is_seed_node_;
}

auto
mcbp_pipeline::debug_string() const -> std::string
{
    std::vector<char> out;
    if (address_.empty()) {
        fmt::format_to(std::back_insert_iterator(out), "dead-server queue");
    } else {
        fmt::format_to(std::back_insert_iterator(out), "address: {}, ", address_);
        fmt::format_to(std::back_insert_iterator(out), "max clients: {}, ", max_clients_);
        fmt::format_to(std::back_insert_iterator(out), "num clients: {}, ", clients_.size());
        fmt::format_to(std::back_insert_iterator(out), "max items: {}, ", max_items_);
    }
    fmt::format_to(std::back_insert_iterator(out), "operation queue: {}", queue_.debug_string());
    return { out.begin(), out.end() };
}

auto
mcbp_pipeline::send_request(std::shared_ptr<mcbp_queue_request> request, std::size_t max_items) -> std::error_code
{
    auto err = queue_.push(std::move(request), max_items);
    if (err == core_errc::operation_queue_closed) {
        return core_errc::pipeline_closed;
    }
    if (err == core_errc::operation_queue_full) {
        return core_errc::pipeline_full;
    }
    if (err) {
        return err;
    }
    return {};
}

auto
mcbp_pipeline::requeue_request(std::shared_ptr<mcbp_queue_request> request) -> std::error_code
{
    return send_request(std::move(request), 0);
}

auto
mcbp_pipeline::send_request(std::shared_ptr<mcbp_queue_request> request) -> std::error_code
{
    return send_request(std::move(request), max_items_);
}

void
mcbp_pipeline::start_clients()
{
    std::scoped_lock lock(clients_mutex_);

    for (std::size_t i; i < max_clients_; ++i) {
        auto client = std::make_shared<mcbp_pipeline_client>(shared_from_this());
        client->start();
        clients_.emplace_back(client);
    }
}

auto
make_mcbp_dead_pipeline(std::size_t max_items) -> std::shared_ptr<mcbp_pipeline>
{
    return std::make_shared<mcbp_pipeline>(route_endpoint{ "", false }, 0, max_items);
}
} // namespace couchbase::new_core
