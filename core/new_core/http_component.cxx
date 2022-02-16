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

#include "http_component.hxx"
#include "impl/http_component.hxx"

namespace couchbase::new_core
{
http_component::http_component(const http_component_properties& properties,
                               const http_client_properties& client_properties,
                               std::shared_ptr<http_multiplexer> multiplexer,
                               std::shared_ptr<tracer_component> tracer)
  : impl_{ properties, client_properties, std::move(multiplexer), std::move(tracer) }
{
}

auto
http_component::do_internal_http_request(std::shared_ptr<http_request> request, bool skip_config_check)
  -> std::pair<http_response, std::error_code>
{
    return impl_->do_internal_http_request(request, skip_config_check);
}
} // namespace couchbase::new_core
