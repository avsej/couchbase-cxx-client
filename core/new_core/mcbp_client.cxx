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

#include "mcbp_client.hxx"
#include "circuit_breaker_configuration.hxx"
#include "lazy_circuit_breaker.hxx"
#include "mcbp_connection.hxx"

#include "../platform/uuid.h"

namespace couchbase::new_core
{
static void
send_canary(const std::shared_ptr<mcbp_client>& /* client */)
{
    /* FIXME: implement canary */
}

auto
make_mcbp_client(const mcbp_client_properties& properties,
                 const circuit_breaker_config& breaker_config,
                 post_complete_error_handler post_error_handler) -> std::shared_ptr<mcbp_client>
{
    auto client = std::make_shared<mcbp_client>(properties, std::move(post_error_handler));
    if (breaker_config.enabled) {
        client->breaker_ = make_lazy_circuit_breaker(breaker_config, [client]() { send_canary(client); });
    }
    return client;
}

mcbp_client::mcbp_client(const mcbp_client_properties& properties, post_complete_error_handler post_error_handler)
  : connection_id_{ properties.client_id + "/" + core::uuid::to_string(core::uuid::random()) }
  , compression_min_size_{ properties.compression_min_size }
  , compression_min_ratio_{ properties.compression_min_ratio }
  , disable_compression_{ properties.disable_compression }
  , post_error_handler_{ std::move(post_error_handler) }
{
}

const std::string&
mcbp_client::address() const
{
    return connection_->remote_address();
}

bool
mcbp_client::supports_feature(mcbp::hello_feature feature) const
{
    return features_.count(feature) > 0;
}

std::system_error
mcbp_client::sasl_list_mechanisms(std::chrono::steady_clock::time_point deadline,
                                  std::function<void(std::vector<auth_mechanism>, std::error_code)> callback)
{
    return {};
}

std::system_error
mcbp_client::sasl_authenticate(std::vector<std::byte> key,
                               std::vector<std::byte> value,
                               std::chrono::steady_clock::time_point deadline,
                               std::function<void(std::vector<std::byte>, std::error_code)> callback)
{
    return {};
}

std::system_error
mcbp_client::sasl_step(std::vector<std::byte> key,
                       std::vector<std::byte> value,
                       std::chrono::steady_clock::time_point deadline,
                       std::function<void(std::error_code)> callback)
{
    return {};
}
} // namespace couchbase::new_core
