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

#include "barrier_frame.hxx"
#include "command_code.hxx"
#include "command_magic.hxx"
#include "durability_level_frame.hxx"
#include "durability_timeout_frame.hxx"
#include "open_tracing_frame.hxx"
#include "preserve_expiry_frame.hxx"
#include "read_units_frame.hxx"
#include "server_duration_frame.hxx"
#include "status_code.hxx"
#include "stream_id_frame.hxx"
#include "unsupported_frame.hxx"
#include "user_impersonation_frame.hxx"
#include "write_units_frame.hxx"

#include <cstddef>
#include <optional>
#include <vector>

namespace couchbase::new_core::mcbp
{
struct packet {
    [[nodiscard]] std::string debug_string() const;

    command_magic magic_{ command_magic::invalid };
    command_code command_{ command_code::invalid };
    std::byte datatype_{};
    status_code status_{ status_code::success };
    std::uint16_t vbucket_{};
    std::uint32_t opaque_{};
    std::uint64_t cas_{};
    std::uint32_t collection_id_{};
    std::vector<std::byte> key_{};
    std::vector<std::byte> extras_{};
    std::vector<std::byte> value_{};

    std::optional<barrier_frame> barrier_frame_{};
    std::optional<durability_level_frame> durability_level_frame_{};
    std::optional<durability_timeout_frame> durability_timeout_frame_{};
    std::optional<stream_id_frame> stream_id_frame_{};
    std::optional<open_tracing_frame> open_tracing_frame_{};
    std::optional<server_duration_frame> server_duration_frame_{};
    std::optional<user_impersonation_frame> user_impersonation_frame_{};
    std::optional<preserve_expiry_frame> preserve_expiry_frame_{};
    std::optional<read_units_frame> read_units_frame_{};
    std::optional<write_units_frame> write_units_frame_{};
    std::vector<unsupported_frame> unsupported_frames_{};
};
} // namespace couchbase::new_core::mcbp
