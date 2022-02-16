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

#include <cstddef>
#include <system_error>

namespace couchbase::new_core
{
class socket_stream
{
  public:
    virtual ~socket_stream() = default;

    [[nodiscard]] virtual auto write(const std::byte* data, std::size_t size) -> std::pair<std::size_t, std::error_code> = 0;
    [[nodiscard]] virtual auto read(std::byte* data, std::size_t size) -> std::pair<std::size_t, std::error_code> = 0;
};
} // namespace couchbase::new_core
