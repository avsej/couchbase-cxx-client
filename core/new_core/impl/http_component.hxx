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

#include "../completion_token.hxx"
#include "../http_client_properties.hxx"
#include "../http_component_properties.hxx"

#include <functional>
#include <memory>

namespace couchbase::new_core::impl
{
class http_multiplexer;
class http_client;
class http_request;
class http_response;
class tracer_component;

using http_callback = std::function<void(std::shared_ptr<http_response> response, std::error_code error)>;

class http_component : public std::enable_shared_from_this<http_component>
{
  public:
    http_component(const http_component_properties& properties,
                   const http_client_properties& client_properties,
                   std::shared_ptr<http_multiplexer> multiplexer,
                   std::shared_ptr<tracer_component> tracer);

    void close();

    auto do_http_request(std::shared_ptr<http_request> request, http_callback callback) -> completion_token;
    auto do_internal_http_request(std::shared_ptr<http_request> request, bool skip_config_check)
      -> std::pair<std::shared_ptr<http_response>, std::error_code>;

  private:
    std::string user_agent_;
    std::shared_ptr<retry_strategy> default_retry_strategy_;
    std::shared_ptr<http_multiplexer> multiplexer_;
    std::shared_ptr<tracer_component> tracer_;
    std::shared_ptr<http_client> client_;
};
} // namespace couchbase::new_core::impl
