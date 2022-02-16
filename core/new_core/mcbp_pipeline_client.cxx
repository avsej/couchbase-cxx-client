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

#include "mcbp_pipeline_client.hxx"
#include "core/logger/logger.hxx"
#include "mcbp_pipeline.hxx"

namespace couchbase::new_core
{
mcbp_pipeline_client::mcbp_pipeline_client(std::shared_ptr<mcbp_pipeline> pipeline)
  : parent_{ std::move(pipeline) }
  , address_{ parent_->address() }
{
}

auto
mcbp_pipeline_client::state() const -> endpoint_state
{
    return state_;
}

auto
mcbp_pipeline_client::error() const -> std::error_code
{
    std::scoped_lock lock(mutex_);
    return connect_error_;
}

void
mcbp_pipeline_client::reassign_to(std::shared_ptr<mcbp_pipeline> new_parent)
{
    std::shared_ptr<mcbp_operation_consumer> old_consumer{ nullptr };
    {
        std::scoped_lock lock(mutex_);
        parent_ = std::move(new_parent);
        std::swap(consumer_, old_consumer);
    }
    if (old_consumer != nullptr) {
        old_consumer->close();
    }
}
} // namespace couchbase::new_core
