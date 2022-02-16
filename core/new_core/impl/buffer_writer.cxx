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

#include "buffer_writer.hxx"

#include <cstring>

namespace couchbase::new_core::impl
{
buffer_writer::buffer_writer(buffer_writer&& other, const buffer_writer::allocator_type alloc) noexcept
  : store{ std::move(other.store), alloc }
  , offset{ other.offset }
{
}

buffer_writer::buffer_writer(const buffer_writer& other, const buffer_writer::allocator_type alloc)
  : store{ other.store, alloc }
  , offset{ other.offset }
{
}

buffer_writer::buffer_writer(std::size_t size, const buffer_writer::allocator_type alloc)
  : store{ size, alloc }
{
}

void
buffer_writer::write(const std::vector<std::byte>& val)
{
    std::memcpy(store.data() + offset, val.data(), val.size());
    offset += val.size();
}

void
buffer_writer::write_frame_header(std::uint8_t type, std::size_t length)
{
    write_byte((std::byte{ type } << 4) | static_cast<std::byte>(length));
}

void
buffer_writer::write_uint64(std::uint64_t val)
{
    write_byte(static_cast<std::byte>(val >> 56));
    write_byte(static_cast<std::byte>(val >> 48));
    write_byte(static_cast<std::byte>(val >> 40));
    write_byte(static_cast<std::byte>(val >> 32));
    write_byte(static_cast<std::byte>(val >> 24));
    write_byte(static_cast<std::byte>(val >> 16));
    write_byte(static_cast<std::byte>(val >> 8));
    write_byte(static_cast<std::byte>(val));
}

void
buffer_writer::write_uint32(std::uint32_t val)
{
    write_byte(static_cast<std::byte>(val >> 24));
    write_byte(static_cast<std::byte>(val >> 16));
    write_byte(static_cast<std::byte>(val >> 8));
    write_byte(static_cast<std::byte>(val));
}

void
buffer_writer::write_uint16(std::uint16_t val)
{
    write_byte(static_cast<std::byte>(val >> 8));
    write_byte(static_cast<std::byte>(val));
}

void
buffer_writer::write_byte(std::byte val)
{
    store[offset] = val;
    ++offset;
}
} // namespace couchbase::new_core::impl
