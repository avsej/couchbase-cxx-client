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

#include "impl/fwd/http_component.hxx"

#include "http_client_properties.hxx"
#include "http_component_properties.hxx"

#include <memory>
#include <system_error>

namespace couchbase::new_core
{
class http_request;
class http_response;
class http_multiplexer;
class tracer_component;

class http_component
{
  public:
    http_component(const http_component_properties& properties,
                   const http_client_properties& client_properties,
                   std::shared_ptr<http_multiplexer> multiplexer,
                   std::shared_ptr<tracer_component> tracer);
    http_component(const http_component& other) = default;
    http_component(http_component&& other) = default;
    auto operator=(const http_component& other) -> http_component& = default;
    auto operator=(http_component&& other) -> http_component& = default;

    auto do_internal_http_request(std::shared_ptr<http_request> request, bool skip_config_check)
      -> std::pair<http_response, std::error_code>;

  private:
    std::shared_ptr<impl::http_component> impl_;

    friend class diagnostics_component;
};
} // namespace couchbase::new_core
