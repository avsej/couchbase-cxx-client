/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright 2022-Present Couchbase, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF
 * ANY KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#pragma once

#include "core_error_context.hxx"

#include <string>
#include <system_error>

namespace couchbase
{
class core_view_error_context_impl;
class core_view_error_context : public core_error_context
{
  public:
    explicit core_view_error_context(core_error_context&& ctx);
    core_view_error_context(core_error_context&& ctx, tao::json::value&& extra);

    core_view_error_context(const core_view_error_context&) = delete;
    core_view_error_context& operator=(const core_view_error_context&) = delete;
    /*
     * Fields specified in SDK-RFC-0058
     */

    [[nodiscard]] auto design_document_name() const -> const std::string&;
    [[nodiscard]] auto view_name() const -> const std::string&;
    [[nodiscard]] auto http_status() const -> std::uint32_t;
    [[nodiscard]] auto http_body() const -> const std::string&;
    [[nodiscard]] auto query_string() const -> const std::vector<std::string>&;

    /*
     * Extensions
     */

    [[nodiscard]] auto client_context_id() const -> const std::string&;
    [[nodiscard]] auto method() const -> const std::string&;
    [[nodiscard]] auto path() const -> const std::string&;
    [[nodiscard]] auto hostname() const -> const std::string&;
    [[nodiscard]] auto port() const -> std::uint16_t;

  private:
    std::unique_ptr<core_view_error_context_impl> impl_;
};
} // namespace couchbase
