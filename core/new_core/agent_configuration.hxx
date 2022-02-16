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

#include "circuit_breaker_configuration.hxx"
#include "compression_configuration.hxx"
#include "config_poller_configuration.hxx"
#include "http_configuration.hxx"
#include "io_configuration.hxx"
#include "mcbp_configuration.hxx"
#include "orphan_reporter_configuration.hxx"
#include "security_configuration.hxx"
#include "seed_configuration.hxx"

#include <memory>
#include <string>

namespace couchbase::new_core
{
class retry_strategy;

// specifies the configuration options for creation of an agent.
struct agent_configuration {
    std::string bucket_name;
    std::string user_agent;

    seed_configuration seed_config;
    security_configuration security_config;
    compression_configuration compression_config;
    config_poller_configuration config_poller_config;
    io_configuration io_config;
    mcbp_configuration mcbp_config;
    http_configuration http_config;
    circuit_breaker_configuration circuit_breaker_config;
    orphan_reporter_configuration orphan_reporter_config;

    std::shared_ptr<retry_strategy> default_retry_strategy;
};
} // namespace couchbase::new_core
