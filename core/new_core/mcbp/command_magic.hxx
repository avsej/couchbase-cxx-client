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

#include <cinttypes>
#include <string>

namespace couchbase::new_core::mcbp
{
/**
 * Represents the magic number that begins the header of every packet and informs the rest of the header format.
 */
enum class command_magic : std::uint8_t {
    invalid = 0x00,

    /**
     * Indicates that packet is a request
     */
    request = 0x80U,

    /**
     * Indicates that packet is a response
     */
    response = 0x81U,

    /**
     * These are private rather than public as the library will automatically
     * switch to and from these magics based on the use of frames within a packet.
     */
    request_ext = 0x08U,
    response_ext = 0x18U,
};

[[nodiscard]] std::string
to_string(command_magic magic);
} // namespace couchbase::new_core::mcbp
