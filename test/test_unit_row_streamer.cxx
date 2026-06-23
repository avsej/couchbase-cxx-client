/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2024. Couchbase, Inc.
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

#include "core/row_streamer.hxx"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("unit: row_streamer_options has streaming defaults", "[unit]")
{
  couchbase::core::row_streamer_options opts{};
  REQUIRE(opts.lexer_depth == 4);
  REQUIRE(opts.high_water_bytes == std::size_t{ 2 } * 1024 * 1024);
  REQUIRE(opts.low_water_bytes == std::size_t{ 512 } * 1024);
  REQUIRE(opts.max_row_bytes == std::size_t{ 64 } * 1024 * 1024);
}
