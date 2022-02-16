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

#include "impl/fwd/tracer_component.hxx"

#include <memory>
#include <system_error>

namespace couchbase::new_core
{
class tracer_component
{
  public:
    tracer_component(const tracer_component& other) = default;
    tracer_component(tracer_component&& other) = default;
    auto operator=(const tracer_component& other) -> tracer_component& = default;
    auto operator=(tracer_component&& other) -> tracer_component& = default;

  private:
    std::shared_ptr<impl::tracer_component> impl_;
};
} // namespace couchbase::new_core
