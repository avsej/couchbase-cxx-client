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

#include <memory_resource>
#include <vector>

namespace couchbase::new_core::impl
{
struct buffer_writer {
    std::pmr::vector<std::byte> store;
    std::size_t offset{ 0 };

    using allocator_type = std::pmr::polymorphic_allocator<char>;

    explicit buffer_writer(std::size_t size, const allocator_type alloc = {});
    buffer_writer(const buffer_writer& other, const allocator_type alloc = {});
    buffer_writer(buffer_writer&& other, const allocator_type alloc = {}) noexcept;
    ~buffer_writer() = default;

    void write_byte(std::byte val);
    void write_uint16(std::uint16_t val);
    void write_uint32(std::uint32_t val);
    void write_uint64(std::uint64_t val);
    void write_frame_header(std::uint8_t type, std::size_t length);
    void write(const std::vector<std::byte>& val);
};
} // namespace couchbase::new_core::impl
