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
#include "test_helper_streaming.hxx"

#include <asio/io_context.hpp>
#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <string>
#include <system_error>
#include <vector>

TEST_CASE("unit: row_streamer_options has streaming defaults", "[unit]")
{
  couchbase::core::row_streamer_options opts{};
  REQUIRE(opts.lexer_depth == 4);
  REQUIRE(opts.high_water_bytes == std::size_t{ 2 } * 1024 * 1024);
  REQUIRE(opts.low_water_bytes == std::size_t{ 512 } * 1024);
  REQUIRE(opts.max_row_bytes == std::size_t{ 64 } * 1024 * 1024);
}

TEST_CASE("unit: row_streamer yields rows then clean end over cached body", "[unit]")
{
  asio::io_context io;
  // A small N1QL-shaped document with two rows and a trailing status.
  std::string doc = R"({"requestID":"x","signature":{"a":"number"},)"
                    R"("results":[{"a":1},{"a":2}],"status":"success","metrics":{}})";
  auto body = test::utils::make_cached_response_body(io, doc);
  couchbase::core::row_streamer streamer{ io, std::move(body), "/results/^" };

  std::vector<std::string> rows;
  std::error_code end_ec{ make_error_code(std::errc::operation_in_progress) };
  std::function<void()> pump = [&]() {
    streamer.next_row([&](std::string row, std::error_code ec) {
      if (ec || row.empty()) {
        end_ec = ec;
        return;
      }
      rows.push_back(std::move(row));
      pump();
    });
  };
  streamer.start([&](std::string /*preamble*/, std::error_code) { pump(); });
  io.run();

  REQUIRE(rows.size() == 2);
  REQUIRE(rows[0].find("\"a\":1") != std::string::npos);
  REQUIRE(!end_ec); // clean end => empty error
}
