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

#include "mcbp/hello_feature.hxx"
#include "mcbp/packet.hxx"

#include <memory>
#include <system_error>

namespace couchbase::new_core
{
class mcbp_connection
{
  public:
    virtual ~mcbp_connection() = default;

    [[nodiscard]] virtual auto local_address() const -> const std::string& = 0;
    [[nodiscard]] virtual auto remote_address() const -> const std::string& = 0;
    [[nodiscard]] virtual auto write_packet(const mcbp::packet& packet) -> std::error_code = 0;
    [[nodiscard]] virtual auto read_packet() -> std::tuple<mcbp::packet, std::size_t, std::error_code> = 0;
    [[nodiscard]] virtual auto close() -> std::error_code = 0;
    virtual void release() = 0;

    virtual void enable_feature(mcbp::hello_feature feature) = 0;
    [[nodiscard]] virtual auto is_feature_enabled(mcbp::hello_feature feature) -> bool = 0;
};

[[nodiscard]] auto
make_mcbp_connection() -> std::shared_ptr<mcbp_connection>;
} // namespace couchbase::new_core
