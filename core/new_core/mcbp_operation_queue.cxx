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

#include "mcbp_operation_queue.hxx"
#include "core/logger/logger.hxx"
#include "errors.hxx"
#include "mcbp_queue_request.hxx"

#include <fmt/core.h>

#include <memory>

namespace couchbase::new_core
{
auto
mcbp_operation_queue::debug_string() const -> std::string
{
    std::scoped_lock lock(mutex_);
    std::vector<char> out;
    fmt::format_to(std::back_insert_iterator(out), "num_items: {}, is_open: {}", items_.size(), is_open_);
    return { out.begin(), out.end() };
}

auto
mcbp_operation_queue::consumer() -> std::shared_ptr<mcbp_operation_consumer>
{
    return std::make_shared<mcbp_operation_consumer>(shared_from_this());
}

void
mcbp_operation_queue::close_consumer(std::shared_ptr<mcbp_operation_consumer> consumer)
{
    std::scoped_lock lock(mutex_);
    consumer->close();
    signal_.notify_all();
}

void
mcbp_operation_queue::close()
{
    std::scoped_lock lock(mutex_);
    is_open_ = false;
    signal_.notify_all();
}

auto
mcbp_operation_queue::push(std::shared_ptr<mcbp_queue_request> request, std::size_t max_items) -> std::error_code
{
    std::scoped_lock lock(mutex_);

    if (!is_open_) {
        return core_errc::operation_queue_closed;
    }

    if (max_items > 0 && items_.size() >= max_items) {
        return core_errc::operation_queue_full;
    }

    if (mcbp_operation_queue * no_queue{ nullptr }; !request->queued_with_.compare_exchange_strong(no_queue, this)) {
        return core_errc::request_already_queued;
    }

    if (request->is_cancelled()) {
        request->queued_with_.exchange(nullptr);
        return core_errc::request_cancelled;
    }

    items_.emplace_back(std::move(request));
    signal_.notify_all();

    return {};
}

auto
mcbp_operation_queue::remove(std::shared_ptr<mcbp_queue_request> request) -> bool
{
    std::scoped_lock lock(mutex_);

    if (!is_open_) {
        return false;
    }

    if (mcbp_operation_queue * this_queue{ nullptr }; !request->queued_with_.compare_exchange_strong(this_queue, nullptr)) {
        return false;
    }

    if (auto item = std::find(items_.begin(), items_.end(), request); item != items_.end()) {
        items_.erase(item);
        return true;
    }

    return false;
}

auto
mcbp_operation_queue::pop(std::shared_ptr<mcbp_operation_consumer> consumer) -> std::shared_ptr<mcbp_queue_request>
{
    std::unique_lock lock(mutex_);

    signal_.wait(lock, [this, consumer]() { return is_open_ && !consumer->is_closed_ && items_.empty(); });

    if (!is_open_ || consumer->is_closed_) {
        return nullptr;
    }

    auto request = items_.front();
    items_.pop_front();
    request->queued_with_.exchange(nullptr);
    return request;
}

void
mcbp_operation_queue::drain(mcbp_operation_queue::drain_callback callback)
{
    for (const auto& request : items_to_drain()) {
        callback(request);
    }
}

auto
mcbp_operation_queue::items_to_drain() -> std::list<std::shared_ptr<mcbp_queue_request>>
{
    std::scoped_lock lock(mutex_);

    if (is_open_) {
        LOG_ERROR("attempted to drain open MCBP operation queue, ignoring");
        return {};
    }

    if (items_.empty()) {
        return {};
    }

    std::list<std::shared_ptr<mcbp_queue_request>> local_items;
    std::swap(local_items, items_);
    for (const auto& request : local_items) {
        request->queued_with_.exchange(nullptr);
    }
    return local_items;
}
} // namespace couchbase::new_core
