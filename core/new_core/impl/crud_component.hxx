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

#include "../ops_fwd.hxx"

#include <memory>

namespace couchbase::new_core::impl
{
class collections_component;
class tracer_component;
class error_map_component;
class bucket_capabilities_verifier;
class retry_strategy;

class crud_component : public std::enable_shared_from_this<crud_component>
{
  public:
    [[nodiscard]] auto get(ops::get_operation operation) -> completion_token;

  private:
    std::shared_ptr<collections_component> collections_component_;
    std::shared_ptr<tracer_component> tracer_component_;
    std::shared_ptr<error_map_component> error_map_component_;
    std::shared_ptr<bucket_capabilities_verifier> feature_verifier_;
    std::shared_ptr<retry_strategy> default_retry_strategy_;
};
} // namespace couchbase::new_core::impl
