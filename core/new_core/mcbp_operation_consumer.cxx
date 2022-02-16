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

#include "mcbp_operation_consumer.hxx"

#include "mcbp_operation_queue.hxx"

namespace couchbase::new_core
{
mcbp_operation_consumer::mcbp_operation_consumer(std::shared_ptr<mcbp_operation_queue> parent)
  : parent_{ std::move(parent) }
{
}

auto
mcbp_operation_consumer::queue() -> std::shared_ptr<mcbp_operation_queue>
{
    return parent_;
}

void
mcbp_operation_consumer::close()
{
    parent_->close_consumer(shared_from_this());
}

auto
mcbp_operation_consumer::pop() -> std::shared_ptr<mcbp_queue_request>
{
    return parent_->pop(shared_from_this());
}
} // namespace couchbase::new_core
