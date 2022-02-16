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

#include "auth_client.hxx"
#include "mcbp/hello_feature.hxx"
#include "mcbp_client_properties.hxx"
#include "noop_circuit_breaker.hxx"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <system_error>

namespace couchbase::new_core
{
class circuit_breaker;
class circuit_breaker_config;
class mcbp_queue_response;
class mcbp_queue_request;
class mcbp_connection;

using post_complete_error_handler = std::function<std::pair<bool, std::error_code>(std::shared_ptr<mcbp_queue_response> response,
                                                                                   std::shared_ptr<mcbp_queue_request> request,
                                                                                   std::error_code error)>;

class mcbp_client
  : public std::enable_shared_from_this<mcbp_client>
  , public auth_client
{
  public:
    mcbp_client(const mcbp_client_properties& properties, post_complete_error_handler post_error_handler);
    auto address() const -> const std::string& override;
    auto supports_feature(mcbp::hello_feature feature) const -> bool override;

    auto sasl_list_mechanisms(std::chrono::steady_clock::time_point deadline,
                              std::function<void(std::vector<auth_mechanism>, std::error_code)> callback) -> std::system_error override;
    auto sasl_authenticate(std::vector<std::byte> key,
                           std::vector<std::byte> value,
                           std::chrono::steady_clock::time_point deadline,
                           std::function<void(std::vector<std::byte>, std::error_code)> callback) -> std::system_error override;
    auto sasl_step(std::vector<std::byte> key,
                   std::vector<std::byte> value,
                   std::chrono::steady_clock::time_point deadline,
                   std::function<void(std::error_code)> callback) -> std::system_error override;

  private:
    std::string connection_id_;
    std::size_t compression_min_size_;
    double compression_min_ratio_;
    bool disable_compression_;

    post_complete_error_handler post_error_handler_;

    std::shared_ptr<mcbp_connection> connection_;
    std::shared_ptr<circuit_breaker> breaker_{ make_noop_circuit_breaker() };
    std::atomic_bool closed_{};
    std::chrono::steady_clock::time_point last_activity_{};
    std::mutex mutex_{};
    std::set<mcbp::hello_feature> features_{};

    friend auto make_mcbp_client(const mcbp_client_properties& properties,
                                 const circuit_breaker_config& breaker_config,
                                 post_complete_error_handler post_error_handler) -> std::shared_ptr<mcbp_client>;
};

auto
make_mcbp_client(const mcbp_client_properties& properties,
                 const circuit_breaker_config& breaker_config,
                 post_complete_error_handler post_error_handler) -> std::shared_ptr<mcbp_client>;
} // namespace couchbase::new_core
