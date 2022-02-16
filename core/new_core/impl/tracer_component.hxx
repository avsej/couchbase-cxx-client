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

#include "tracer_handler.hxx"

#include "../request_span_context.hxx"

#include <memory>
#include <string>

namespace couchbase::new_core
{
class request_tracer;
} // namespace couchbase::new_core

namespace couchbase::new_core::impl
{
class tracer_component : public std::enable_shared_from_this<tracer_component>
{
  public:
    [[nodiscard]] auto create_operation_trace(std::string service, std::shared_ptr<request_span_context> parent_context) -> tracer_handler;
    [[nodiscard]] auto start_http_dispatch_span(std::string service, std::string operation) -> tracer_handler;
    [[nodiscard]] auto stop_http_dispatch_span(std::string service, std::string operation) -> tracer_handler;
    [[nodiscard]] auto start_command_trace(std::string service, std::string operation) -> tracer_handler;
    [[nodiscard]] auto start_network_trace(std::string service, std::string operation) -> tracer_handler;
    [[nodiscard]] auto response_value_record(std::string service, std::string operation) -> tracer_handler;
    [[nodiscard]] auto start_telemetry_handler(std::string service, std::string operation) -> tracer_handler;

  private:
    std::shared_ptr <
};
} // namespace couchbase::new_core::impl
