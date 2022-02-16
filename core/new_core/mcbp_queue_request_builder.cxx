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

#include "mcbp_queue_request_builder.hxx"

namespace couchbase::new_core
{
mcbp_queue_request_builder::mcbp_queue_request_builder(mcbp::command_magic magic, mcbp::command_code opcode, mcbp_callback callback)
  : request_{ std::make_shared<mcbp_queue_request>(magic, opcode, std::move(callback)) }
{
}

auto
mcbp_queue_request_builder::retry_strategy(std::shared_ptr<class retry_strategy> strategy) -> mcbp_queue_request_builder&
{
    request_->retry_strategy_ = std::move(strategy);
    return *this;
}

auto
make_mcbp_queue_request(mcbp::command_magic magic, mcbp::command_code opcode, mcbp_callback callback) -> mcbp_queue_request_builder
{
    return { magic, opcode, std::move(callback) };
}
} // namespace couchbase::new_core
