/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2026-Present Couchbase, Inc.
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

#include "test_helper_integration.hxx"

#include <couchbase/cluster.hxx>
#include <couchbase/error_codes.hxx>

using namespace std::literals::chrono_literals;

TEST_CASE("integration: cluster wait_until_ready", "[integration]")
{
  test::utils::integration_test_guard integration;

  auto cluster = integration.public_cluster();

  SECTION("reaches online state")
  {
    couchbase::wait_until_ready_options options{};
    options.service_types({ couchbase::service_type::key_value });
    auto err = cluster.wait_until_ready(10s, options).get();
    REQUIRE_SUCCESS(err.ec());
  }

  SECTION("offline is not a valid target")
  {
    couchbase::wait_until_ready_options options{};
    options.desired_state(couchbase::cluster_state::offline);
    auto err = cluster.wait_until_ready(10s, options).get();
    REQUIRE(err.ec() == couchbase::errc::common::invalid_argument);
  }
}

TEST_CASE("integration: bucket wait_until_ready", "[integration]")
{
  test::utils::integration_test_guard integration;

  auto cluster = integration.public_cluster();

  SECTION("existing bucket reaches online state")
  {
    couchbase::wait_until_ready_options options{};
    options.service_types({ couchbase::service_type::key_value });
    auto err = cluster.bucket(integration.ctx.bucket).wait_until_ready(10s, options).get();
    REQUIRE_SUCCESS(err.ec());
  }

  SECTION("missing bucket times out")
  {
    auto err = cluster.bucket("this_bucket_does_not_exist").wait_until_ready(2s).get();
    REQUIRE(err.ec() == couchbase::errc::common::unambiguous_timeout);
  }
}
