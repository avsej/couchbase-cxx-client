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

#include "auth_mechanism.hxx"
#include "mcbp/hello_feature.hxx"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <system_error>
#include <vector>

namespace couchbase::new_core
{
class auth_client
{
  public:
    virtual ~auth_client() = default;

    [[nodiscard]] virtual auto address() const -> const std::string& = 0;
    [[nodiscard]] virtual auto supports_feature(mcbp::hello_feature feature) const -> bool = 0;

    [[nodiscard]] virtual auto sasl_list_mechanisms(
      std::chrono::steady_clock::time_point deadline,
      std::function<void(std::vector<auth_mechanism> mechanisms, std::error_code error)> callback) -> std::system_error = 0;

    [[nodiscard]] virtual auto sasl_authenticate(std::vector<std::byte> key,
                                                 std::vector<std::byte> value,
                                                 std::chrono::steady_clock::time_point deadline,
                                                 std::function<void(std::vector<std::byte> response, std::error_code error)> callback)
      -> std::system_error = 0;

    [[nodiscard]] virtual auto sasl_step(std::vector<std::byte> key,
                                         std::vector<std::byte> value,
                                         std::chrono::steady_clock::time_point deadline,
                                         std::function<void(std::error_code error)> callback) -> std::system_error = 0;
};
} // namespace couchbase::new_core
