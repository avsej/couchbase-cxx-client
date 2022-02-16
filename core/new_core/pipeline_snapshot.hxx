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

#include "mcbp_multiplexer_state.hxx"
#include "mcbp_pipeline.hxx"

#include <cstddef>
#include <functional>
#include <memory>

namespace couchbase::new_core
{
class pipeline_snapshot
{
  public:
    explicit pipeline_snapshot(std::shared_ptr<mcbp_multiplexer_state> state)
      : state_{ std::move(state) }
    {
    }

    [[nodiscard]] auto number_of_pipelines() const -> std::size_t;

    void iterate(std::size_t node_index, std::function<bool(std::shared_ptr<mcbp_pipeline>)> handler);

  private:
    std::shared_ptr<mcbp_multiplexer_state> state_{};
};
} // namespace couchbase::new_core
