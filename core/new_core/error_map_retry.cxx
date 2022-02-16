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

#include "error_map_retry.hxx"

#include <couchbase/error_codes.hxx>

#include <fmt/core.h>

namespace couchbase::new_core
{
auto
error_map_retry::calculate_duration(std::size_t retry_count) const -> std::chrono::milliseconds
{
    std::chrono::milliseconds duration;

    if (retry_count == 0) {
        duration = after;
    } else {
        if (strategy == "constant") {
            duration = interval;
        } else if (strategy == "linear") {
            duration = interval * retry_count;
        } else if (strategy == "exponential") {
            duration = interval;
            for (std::size_t i = 0; i < retry_count - 1; ++i) {
                duration *= interval.count();

                if (ceil > std::chrono::milliseconds::zero() && duration > ceil) {
                    duration = ceil;
                    break;
                }
            }
        } else {
            throw std::system_error(errc::common::invalid_argument,
                                    fmt::format("invalid retry strategy specified in error map: \"{}\"", strategy));
        }
    }

    if (ceil > std::chrono::milliseconds::zero() && duration > ceil) {
        duration = ceil;
    }

    return duration;
}
} // namespace couchbase::new_core
