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

#include "../error_map_error.hxx"
#include "../key_value_error.hxx"
#include "../mcbp/status_code.hxx"

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace couchbase::new_core
{
class mcbp_queue_request;
class mcbp_queue_response;
} // namespace couchbase::new_core

namespace couchbase::new_core::impl
{
class error_map;

class error_map_component : public std::enable_shared_from_this<error_map_component>
{
  public:
    explicit error_map_component(std::string bucket_name);

    void store_error_map(const std::vector<std::byte>& error_map_bytes);

    [[nodiscard]] auto should_retry(mcbp::status_code code) const -> bool;
    [[nodiscard]] auto enhance_kv_error(std::error_code error,
                                        std::shared_ptr<mcbp_queue_response> response,
                                        std::shared_ptr<mcbp_queue_request> request) const -> key_value_error;

  private:
    [[nodiscard]] auto get_error_map_data(mcbp::status_code code) const -> const error_map_error*;

    std::string bucket_name_;
    std::shared_ptr<error_map> error_map_{};
    mutable std::mutex error_map_mutex_{};
};
} // namespace couchbase::new_core::impl
