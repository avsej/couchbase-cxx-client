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

#include "http_component.hxx"

#include "../http_multiplexer.hxx"
#include "../tracer_component.hxx"

namespace couchbase::new_core::impl
{
http_component::http_component(const http_component_properties& properties,
                               const http_client_properties& client_properties,
                               std::shared_ptr<http_multiplexer> multiplexer,
                               std::shared_ptr<tracer_component> tracer)
  : user_agent_{ properties.user_agent }
  , default_retry_strategy_{ properties.default_retry_strategy }
  , multiplexer_{ std::move(multiplexer) }
  , tracer_{ std::move(tracer) }
  , client_{ /* FIXME */ }
{
}

void
http_component::close()
{
}

auto
http_component::do_http_request(std::shared_ptr<http_request> request, http_callback callback) -> completion_token
{
    return completion_token();
}

auto
http_component::do_internal_http_request(std::shared_ptr<http_request> request, bool skip_config_check)
  -> std::pair<std::shared_ptr<http_response>, std::error_code>
{
    return std::pair<std::shared_ptr<http_response>, std::error_code>();
}
} // namespace couchbase::new_core::impl
