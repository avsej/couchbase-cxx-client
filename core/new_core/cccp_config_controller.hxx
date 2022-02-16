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

#include "cccp_poller_properties.hxx"
#include "config_management_component.hxx"
#include "mcbp_dispatcher.hxx"

#include <asio.hpp>

#include <chrono>
#include <memory>
#include <mutex>

namespace couchbase::new_core
{
class cccp_config_controller
{
  public:
    cccp_config_controller(const cccp_poller_properties& properties,
                           std::shared_ptr<asio::io_context> io,
                           std::shared_ptr<mcbp_dispatcher> dispatcher,
                           config_management_component config_manager,
                           std::function<bool(std::error_code ec)> is_fallback_error);

    [[nodiscard]] auto error() const -> std::error_code;

    void stop();
    void done();
    [[nodiscard]] auto do_loop() -> std::error_code;

  private:
    void set_error(std::error_code error);
    [[nodiscard]] auto get_cluster_config(std::shared_ptr<mcbp_pipeline> pipeline) -> std::pair<std::vector<std::byte>, std::error_code>;

    std::shared_ptr<asio::io_context> io_;
    std::shared_ptr<mcbp_dispatcher> dispatcher_;
    config_management_component config_management_component_;
    std::function<bool(std::error_code ec)> is_fallback_error_;
    std::chrono::milliseconds poll_period_;
    std::chrono::milliseconds max_wait_;
    std::error_code fetch_error_{};
    mutable std::mutex fetch_error_mutex_{};
};
} // namespace couchbase::new_core
