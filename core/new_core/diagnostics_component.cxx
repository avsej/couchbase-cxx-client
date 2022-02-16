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

#include "diagnostics_component.hxx"

#include "impl/diagnostics_component.hxx"

namespace couchbase::new_core
{
diagnostics_component::diagnostics_component(std::shared_ptr<mcbp_multiplexer> mcbp_mux,
                                             std::shared_ptr<http_multiplexer> http_mux,
                                             http_component http,
                                             std::string bucket_name,
                                             std::shared_ptr<retry_strategy> default_retry_strategy,
                                             std::shared_ptr<poller_error_provider> error_provider)
  : impl_{ std::make_shared<impl::diagnostics_component>(std::move(mcbp_mux),
                                                         std::move(http_mux),
                                                         std::move(http.impl_),
                                                         std::move(bucket_name),
                                                         std::move(default_retry_strategy),
                                                         std::move(error_provider)) }
{
}

auto
diagnostics_component::diagnostics(const diagnostics_options& options) -> std::pair<diagnostics_info, std::error_code>
{
    return impl_->diagnostics(options);
}

auto
diagnostics_component::ping(ops::ping_operation operation) -> completion_token
{
    return impl_->ping(std::move(operation));
}

auto
diagnostics_component::wait_until_ready(ops::wait_until_ready_operation operation) -> completion_token
{
    return impl_->wait_until_ready(std::move(operation));
}
} // namespace couchbase::new_core
