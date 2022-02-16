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

#include "impl/agent.hxx"

#include "agent.hxx"

namespace couchbase::new_core
{
agent::agent(const agent_configuration& config)
  : impl_(std::make_shared<impl::agent>(config))
{
}

auto
agent::execute(ops::get_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::get_and_touch_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::get_and_lock_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::get_one_replica_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::touch_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::unlock_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::remove_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::insert_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::upsert_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::replace_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::append_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::prepend_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::increment_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::decrement_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::get_random_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::get_meta_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::set_meta_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::remove_meta_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::stats_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::observe_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::observe_vbucket_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::lookup_in_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::mutate_in_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::query_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::prepared_query_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::analytics_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::search_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::view_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::freeform_http_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::get_collection_manifest_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::get_all_collection_manifests_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::get_collection_id_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::execute(ops::ping_operation operation) -> completion_token
{
    return impl_->execute(std::move(operation));
}

auto
agent::diagnostics(const diagnostics_options& options) -> completion_token
{
    return impl_->diagnostics(options);
}
} // namespace couchbase::new_core
