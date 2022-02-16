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

#include "cccp_config_controller.hxx"

#include "errors.hxx"
#include "impl/config_utils.hxx"

#include "core/logger/logger.hxx"
#include "fail_fast_retry_strategy.hxx"
#include "mcbp_queue_request_builder.hxx"

#include <spdlog/fmt/bin_to_hex.h>

#include <random>
#include <utility>

namespace couchbase::new_core
{
cccp_config_controller::cccp_config_controller(const cccp_poller_properties& properties,
                                               std::shared_ptr<asio::io_context> io,
                                               std::shared_ptr<mcbp_dispatcher> dispatcher,
                                               config_management_component config_manager,
                                               std::function<bool(std::error_code ec)> is_fallback_error)
  : io_{ std::move(io) }
  , dispatcher_{ std::move(dispatcher) }
  , config_management_component_{ std::move(config_manager) }
  , is_fallback_error_{ std::move(is_fallback_error) }
  , poll_period_(properties.poll_period)
  , max_wait_(properties.max_wait)
{
}

auto
cccp_config_controller::error() const -> std::error_code
{
    std::scoped_lock lock(fetch_error_mutex_);
    return fetch_error_;
}

void
cccp_config_controller::set_error(std::error_code error)
{
    std::scoped_lock lock(fetch_error_mutex_);
    fetch_error_ = error;
}

void
cccp_config_controller::stop()
{
    /* TODO: implement it */
}

void
cccp_config_controller::done()
{
    /* TODO: implement it */
}

auto
cccp_config_controller::get_cluster_config(std::shared_ptr<mcbp_pipeline> pipeline) -> std::pair<std::vector<std::byte>, std::error_code>
{
    std::vector<std::byte> config_bytes;
    std::error_code error;
    auto req = make_mcbp_queue_request(mcbp::command_magic::request,
                                       mcbp::command_code::get_cluster_config,
                                       [&config_bytes, &error](std::shared_ptr<mcbp_queue_response> response,
                                                               std::shared_ptr<mcbp_queue_request> /* request */,
                                                               std::error_code ec) {
                                           if (response) {
                                               config_bytes = std::move(response->value_);
                                           }
                                           error = ec;
                                       })
                 .retry_strategy(make_fail_fast_retry_strategy())
                 .build();
    auto err = pipeline->send_request(req);
    if (err) {
        return { {}, err };
    }
}

std::error_code
cccp_config_controller::do_loop()
{
    (void)poll_period_;
    (void)max_wait_;
    /* TODO: implement it */
    LOG_DEBUG("CCCP loop starting");
    std::size_t node_index = std::numeric_limits<std::size_t>::max();
    // The first time that we loop we want to skip any sleep so that we can try get a config and bootstrapped ASAP.
    bool first_loop = true;

    while (true) {
        if (!first_loop) {
            // wait for either the agent to be shut down, or our tick time to expire
            // select {
            //   case <-ccc.looperStopSig:
            //   return nil
            //   case <-time.After(tickTime):
            // }
        }
        first_loop = false;

        auto [iter, err] = dispatcher_->get_pipeline_snapshot();
        if (err) {
            // if we have an error, it indicates the client is shut down
            break;
        }

        auto num_nodes = iter.number_of_pipelines();
        if (num_nodes == 0) {
            LOG_DEBUG("cccp_poll: no nodes available to poll, return upstream");
            return core_errc::no_cccp_hosts;
        }

        if (node_index == std::numeric_limits<std::size_t>::max()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<std::uint64_t> dis(0, num_nodes);
            node_index = dis(gen);
        }

        std::error_code found_error{};
        std::optional<config_value> found_config{};
        iter.iterate(node_index,
                     [&node_index, num_nodes, this, &found_error, &found_config](std::shared_ptr<mcbp_pipeline> pipeline) -> bool {
                         node_index = (node_index + 1) % num_nodes;
                         auto [cccp_bytes, e1] = get_cluster_config(pipeline);
                         if (e1) {
                             if (is_fallback_error_(e1)) {
                                 // this error is indicative of a memcached bucket which we can't handle so return the error
                                 LOG_WARNING("cccp_poll: CCCP not supported, returning error upstream: {}", e1.message());
                                 found_error = e1;
                                 return true;
                             }

                             // only log the error at warn if it's unexpected.
                             // if we cancelled the request or we're shutting down the connection, then it's not really unexpected
                             set_error(e1);
                             if (e1 == core_errc::request_cancelled || e1 == core_errc::shutdown) {
                                 LOG_DEBUG("cccp_poll: CCCP request was cancelled or connection was shutdown: {}", e1.message());
                                 return true;
                             }

                             LOG_WARNING("cccp_poll: failed to retrieve CCCP config: {}", e1.message());
                             return false;
                         }

                         set_error({});

                         LOG_TRACE("cccp_poll: got block: {:a}", spdlog::to_hex(cccp_bytes));

                         auto [config, e2] = utils::parse_config(cccp_bytes, pipeline->host());
                         if (e2) {
                             LOG_WARNING("cccp_poll: failed to parse CCCP config: {}", e2.message());
                             return false;
                         }

                         found_config.emplace(std::move(config));
                         return true;
                     });

        if (found_error) {
            return found_error;
        }

        if (!found_config) {
            // only log the error at warn if it's unexpected
            // if we cancelled the request then we're shutting down adn this isn't unexpected
            if (auto e3 = error(); e3 == core_errc::request_cancelled || e3 == core_errc::shutdown) {
                LOG_DEBUG("cccp_poll: CCCP request was cancelled: {}", e3.message());
            } else {
                LOG_WARNING("cccp_poll: failed to retrieve config from any node: {}", e3.message());
            }
            continue;
        }

        LOG_WARNING("cccp_poll: received new config");
        config_management_component_->on_new_config(found_config.value());
    }
    return {};
}

} // namespace couchbase::new_core
