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

#include "core_operation_info.hxx"

#include <tao/json/forward.hpp>

#include <memory>

namespace couchbase
{
class core_error_context_impl;

class core_error_context
{
  public:
    core_error_context(core_operation_info info, tao::json::value&& ctx);
    core_error_context(core_error_context&& other, tao::json::value&& extra);

    core_error_context(core_error_context&& other) noexcept;
    core_error_context& operator=(core_error_context&& other) noexcept;

    core_error_context(const core_error_context&) = delete;
    core_error_context& operator=(const core_error_context&) = delete;

    [[nodiscard]] auto to_json_string() const -> std::string;

    [[nodiscard]] auto ec() const -> std::error_code;
    [[nodiscard]] auto last_dispatched_to() const -> const std::optional<std::string>&;
    [[nodiscard]] auto last_dispatched_from() const -> const std::optional<std::string>&;
    [[nodiscard]] auto retry_attempts() const -> std::size_t;
    [[nodiscard]] auto retry_reasons() const -> const std::set<retry_reason>&;

  protected:
    std::unique_ptr<core_error_context_impl> impl_;
};
} // namespace couchbase
