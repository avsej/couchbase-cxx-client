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

#include "core/new_core/mcbp_connection.hxx"

#include "socket_stream.hxx"

#include <array>
#include <memory_resource>
#include <set>

namespace couchbase::new_core::impl
{

class mcbp_connection
  : public new_core::mcbp_connection
  , public std::enable_shared_from_this<mcbp_connection>
{
  public:
    auto local_address() const -> const std::string& override;
    auto remote_address() const -> const std::string& override;
    auto write_packet(const mcbp::packet& packet) -> std::error_code override;
    auto read_packet() -> std::tuple<mcbp::packet, std::size_t, std::error_code> override;
    auto close() -> std::error_code override;
    void release() override;
    void enable_feature(mcbp::hello_feature feature) override;
    auto is_feature_enabled(mcbp::hello_feature feature) -> bool override;

  private:
    std::string local_address_{};
    std::string remote_address_{};
    bool collections_enabled_{ false };
    std::set<mcbp::hello_feature> enabled_features_{};
    std::array<std::byte, 24> header_buffer_{};
    std::pmr::synchronized_pool_resource memory_pool_{};
    std::shared_ptr<socket_stream> stream_{};
};

} // namespace couchbase::new_core::impl
