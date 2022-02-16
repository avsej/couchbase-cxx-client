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

#include "mcbp_queue_request.hxx"
#include "mcbp_queue_response.hxx"

#include <atomic>

namespace couchbase::new_core
{
mcbp_queue_request::mcbp_queue_request(mcbp::command_magic magic, mcbp::command_code opcode, mcbp_queue_callback callback)
  : mcbp::packet{ magic, opcode }
  , callback_{ std::move(callback) }
{
}

std::size_t
mcbp_queue_request::retry_attempts() const
{
    std::scoped_lock lock(retry_mutex_);
    return retry_count_;
}

std::string
mcbp_queue_request::identifier() const
{
    return std::to_string(opaque_);
}

bool
mcbp_queue_request::idempotent() const
{
    return mcbp::is_idempotent(command_);
}

std::set<retry_reason>
mcbp_queue_request::retry_reasons() const
{
    std::scoped_lock lock(retry_mutex_);
    return retry_reasons_;
}

void
mcbp_queue_request::record_retry_attempt(retry_reason reason)
{
}

auto
mcbp_queue_request::is_cancelled() const -> bool
{
    return is_completed_.load();
}

auto
mcbp_queue_request::retries() const -> std::pair<std::size_t, std::set<retry_reason>>
{
    std::scoped_lock lock(retry_mutex_);
    return { retry_count_, retry_reasons_ };
}

auto
mcbp_queue_request::connection_info() const -> mcbp_queue_request_connection_info
{
    std::scoped_lock lock(connection_info_mutex_);
    return connection_info_;
}

void
mcbp_queue_request::cancel()
{
    // FIXME:
}
} // namespace couchbase::new_core
