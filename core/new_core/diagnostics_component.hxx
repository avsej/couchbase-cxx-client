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

#include "impl/fwd/diagnostics_component.hxx"

#include "completion_token.hxx"
#include "diagnostics_info.hxx"
#include "diagnostics_options.hxx"
#include "http_component.hxx"
#include "ops/ping_operation.hxx"
#include "ops/wait_until_ready_operation.hxx"

#include <chrono>
#include <memory>
#include <system_error>

namespace couchbase::new_core
{
class mcbp_multiplexer;
class http_multiplexer;
class retry_strategy;
class poller_error_provider;

class diagnostics_component
{
  public:
    diagnostics_component(std::shared_ptr<mcbp_multiplexer> mcbp_mux,
                          std::shared_ptr<http_multiplexer> http_mux,
                          http_component http,
                          std::string bucket_name,
                          std::shared_ptr<retry_strategy> default_retry_strategy,
                          std::shared_ptr<poller_error_provider> error_provider);
    diagnostics_component(const diagnostics_component& other) = default;
    diagnostics_component(diagnostics_component&& other) = default;
    auto operator=(const diagnostics_component& other) -> diagnostics_component& = default;
    auto operator=(diagnostics_component&& other) -> diagnostics_component& = default;

    [[nodiscard]] auto diagnostics(const diagnostics_options& options) -> std::pair<diagnostics_info, std::error_code>;
    [[nodiscard]] auto ping(ops::ping_operation operation) -> completion_token;
    [[nodiscard]] auto wait_until_ready(ops::wait_until_ready_operation operation) -> completion_token;

  private:
    std::shared_ptr<impl::diagnostics_component> impl_;
};
} // namespace couchbase::new_core
