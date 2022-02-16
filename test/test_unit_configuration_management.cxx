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

#include "test_helper.hxx"

#include "core/utils/json.hxx"

#include "test/data/bucket_config_with_external_addresses_json.hxx"

#include "core/config/cccp_configuration_controller.hxx"
#include "core/config/configuration_management_component.hxx"
#include "core/config/errors.hxx"

#include <couchbase/error_codes.hxx>

#include <asio.hpp>

#include <thread>

using namespace couchbase::core;

class last_config_holder : public config::route_config_watcher
{
  public:
    ~last_config_holder() override = default;

    void on_new_route_config(std::shared_ptr<config::route_config> config) override
    {
        last_config_ = config;
    }

    [[nodiscard]] auto last_config() const
    {
        return last_config_;
    }

  private:
    std::shared_ptr<config::route_config> last_config_{};
};

TEST_CASE("unit: configuration manager can parse config with external addresses", "[unit]")
{
    const std::string localhost{ "127.0.0.1" };
    config::configuration_manager_properties properties{};
    config::configuration_management_component mgr{ properties };

    auto watcher = std::make_shared<last_config_holder>();

    mgr.add_config_watcher(watcher);
    mgr.on_new_config(config::config_value{ utils::json::parse(bucket_config_with_external_addresses_json_string), localhost });

    auto config = watcher->last_config();
    REQUIRE(config != nullptr);

    REQUIRE(config->is_valid());
    REQUIRE(config->bucket_type() == config::bucket_type::couchbase);
    REQUIRE(config->has_vbucket_map());
    REQUIRE_FALSE(config->is_gcccp_config());
    REQUIRE(config->rev_id() == 1073);
    REQUIRE(config->key_value_endpoints().non_ssl_endpoints.size() == 3);

    REQUIRE(config->vb_map()->is_valid());
    REQUIRE(config->vb_map()->number_of_replicas() == 1);
    REQUIRE(config->vb_map()->number_of_vbuckets() == 1024);
    REQUIRE(config->vb_map()->vbucket_by_key("foo") == 115);
    REQUIRE(config->vb_map()->vbucket_by_key("bar") == 767);

    {
        auto [node, err] = config->vb_map()->node_by_key("foo", 0 /* active */);
        REQUIRE_FALSE(err);
        REQUIRE(node == 0);
    }
    {
        auto [node, err] = config->vb_map()->node_by_key("foo", 1 /* replica */);
        REQUIRE_FALSE(err);
        REQUIRE(node == 1);
    }
    {
        auto [node, err] = config->vb_map()->node_by_key("bar", 0 /* active */);
        REQUIRE_FALSE(err);
        REQUIRE(node == 2);
    }
    {
        auto [node, err] = config->vb_map()->node_by_key("bar", 1 /* replica */);
        REQUIRE_FALSE(err);
        REQUIRE(node == 0);
    }
}

static auto
is_polling_fallback_error(std::error_code ec) -> bool
{
    return ec == couchbase::errc::key_value::document_not_found || ec == couchbase::errc::common::unsupported_operation ||
           ec == couchbase::errc::common::bucket_not_found || ec == couchbase::core::core_errc::no_cccp_hosts;
}

TEST_CASE("unit: configuration controller polls for updates", "[unit]")
{
    auto io = std::make_shared<asio::io_context>();
    std::thread io_thread([io]() { io->run(); });

    auto manager = std::make_shared<config::configuration_management_component>(config::configuration_manager_properties{});
    manager->add_config_watcher(std::make_shared<last_config_holder>());

    auto dispatcher = std::make_shared<config::mcbp_dispatcher>(io);

    config::cccp_configuration_controller controller(
      config::cccp_poller_properties{
        std::chrono::milliseconds{ 200 },
        std::chrono::milliseconds{ 500 },
      },
      io,
      dispatcher,
      manager,
      is_polling_fallback_error);
    REQUIRE_SUCCESS(controller.do_loop());

    io_thread.join();
}
