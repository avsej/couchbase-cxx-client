/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2020-2021 Couchbase, Inc.
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

#include "cluster_options.hxx"
#include "core/topology/endpoint.hxx"

#include <string>
#include <vector>

namespace couchbase::core
{
namespace utils
{
struct connection_string;
} // namespace utils

struct cluster_credentials {
  std::string username{};
  std::string password{};
  std::string certificate_path{};
  std::string key_path{};
  std::optional<std::vector<std::string>> allowed_sasl_mechanisms{};

  [[nodiscard]] auto uses_certificate() const -> bool;
};

namespace topology
{
struct configuration;
} // namespace topology

struct origin {
  origin() = default;
  ~origin() = default;

  origin(origin&& other) = default;
  origin(const origin& other);
  origin(origin other, const topology::configuration& config);
  origin(cluster_credentials auth,
         const std::string& hostname,
         std::uint16_t port,
         cluster_options options);
  origin(cluster_credentials auth,
         const std::string& hostname,
         const std::string& port,
         cluster_options options);
  origin(cluster_credentials auth, const utils::connection_string& connstr);
  auto operator=(origin&& other) -> origin& = default;
  auto operator=(const origin& other) -> origin&;

  [[nodiscard]] auto username() const -> const std::string&;
  [[nodiscard]] auto password() const -> const std::string&;
  [[nodiscard]] auto certificate_path() const -> const std::string&;
  [[nodiscard]] auto key_path() const -> const std::string&;

  [[nodiscard]] auto get_hostnames() const -> std::vector<std::string>;
  [[nodiscard]] auto get_nodes() const -> std::vector<std::string>;

  void set_nodes(const std::vector<topology::endpoint>& nodes);
  void set_nodes_from_config(const topology::configuration& config);

  [[nodiscard]] auto next_address() -> topology::endpoint;

  [[nodiscard]] auto exhausted() const -> bool;

  void restart();

  [[nodiscard]] auto options() const -> const couchbase::core::cluster_options&;
  [[nodiscard]] auto options() -> couchbase::core::cluster_options&;
  [[nodiscard]] auto credentials() const -> const couchbase::core::cluster_credentials&;
  [[nodiscard]] auto to_json() const -> std::string;

private:
  couchbase::core::cluster_options options_{};
  cluster_credentials credentials_{};
  std::vector<topology::endpoint> nodes_{};
  std::vector<topology::endpoint>::iterator next_node_{};
  bool exhausted_{ false };
};

} // namespace couchbase::core
