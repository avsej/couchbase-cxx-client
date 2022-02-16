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

#include "endpoint_state.hxx"

#include <atomic>
#include <memory>

namespace couchbase::new_core
{
class mcbp_pipeline;
class mcbp_operation_consumer;
class mcbp_client;

class mcbp_pipeline_client
{
  public:
    explicit mcbp_pipeline_client(std::shared_ptr<mcbp_pipeline> pipeline);

    [[nodiscard]] auto state() const -> endpoint_state;
    [[nodiscard]] auto error() const -> std::error_code;

    void reassign_to(std::shared_ptr<mcbp_pipeline> new_parent);

  private:
    std::shared_ptr<mcbp_pipeline> parent_;
    std::string address_;
    std::shared_ptr<mcbp_client> client_{};
    std::shared_ptr<mcbp_operation_consumer> consumer_{};
    mutable std::mutex mutex_{};
    std::error_code connect_error_{};
    std::atomic<endpoint_state> state_{ endpoint_state::disconnected };
};
} // namespace couchbase::new_core
