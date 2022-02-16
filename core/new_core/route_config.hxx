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

#include "bucket_capabilities.hxx"
#include "bucket_type.hxx"
#include "cluster_capabilities.hxx"
#include "ketama_continuum.hxx"
#include "route_endpoints.hxx"
#include "vbucket_map.hxx"

#include <memory>
#include <string>

namespace couchbase::new_core
{
struct route_config {
  public:
    route_config() = default;

    route_config(bucket_type type,
                 std::int64_t rev_id,
                 std::int64_t rev_epoch,
                 std::string uuid,
                 std::string name,
                 route_endpoints key_value_endpoints,
                 route_endpoints views_endpoints,
                 route_endpoints management_endpoints,
                 route_endpoints query_endpoints,
                 route_endpoints search_endpoints,
                 route_endpoints analytics_endpoints,
                 route_endpoints eventing_endpoints,
                 cluster_capabilities cluster_caps,
                 bucket_capabilities bucket_caps);

    route_config(bucket_type type,
                 std::int64_t rev_id,
                 std::int64_t rev_epoch,
                 std::string uuid,
                 std::string name,
                 route_endpoints key_value_endpoints,
                 route_endpoints views_endpoints,
                 route_endpoints management_endpoints,
                 route_endpoints query_endpoints,
                 route_endpoints search_endpoints,
                 route_endpoints analytics_endpoints,
                 route_endpoints eventing_endpoints,
                 cluster_capabilities cluster_caps,
                 bucket_capabilities bucket_caps,
                 std::shared_ptr<vbucket_map> vb_map);

    route_config(bucket_type type,
                 std::int64_t rev_id,
                 std::int64_t rev_epoch,
                 std::string uuid,
                 std::string name,
                 route_endpoints key_value_endpoints,
                 route_endpoints views_endpoints,
                 route_endpoints management_endpoints,
                 route_endpoints query_endpoints,
                 route_endpoints search_endpoints,
                 route_endpoints analytics_endpoints,
                 route_endpoints eventing_endpoints,
                 cluster_capabilities cluster_caps,
                 bucket_capabilities bucket_caps,
                 std::shared_ptr<ketama_continuum> ketama_map);

    [[nodiscard]] auto is_valid() const -> bool;
    [[nodiscard]] auto is_gcccp_config() const -> bool;
    [[nodiscard]] auto is_newer_than(const route_config& other) const -> bool;
    [[nodiscard]] auto key_value_endpoints() const -> const route_endpoints&;
    [[nodiscard]] auto management_endpoints() const -> const route_endpoints&;
    [[nodiscard]] auto rev_id() const -> std::int64_t;
    [[nodiscard]] auto name() const -> const std::string&;
    [[nodiscard]] auto type() const -> bucket_type;
    [[nodiscard]] auto has_vbucket_map() const -> bool;
    [[nodiscard]] auto vb_map() const -> std::shared_ptr<vbucket_map>;
    [[nodiscard]] auto ketama_map() const -> std::shared_ptr<ketama_continuum>;
    [[nodiscard]] auto debug_string() const -> std::string;
    [[nodiscard]] auto bucket_caps() const -> bucket_capabilities;
    [[nodiscard]] auto cluster_caps() const -> cluster_capabilities;

  private:
    bucket_type bucket_type_{ bucket_type::invalid };
    std::int64_t rev_epoch_{};
    std::int64_t rev_id_{ -1 };
    std::string uuid_{};
    std::string name_{};
    route_endpoints key_value_endpoints_{};
    route_endpoints views_endpoints_{};
    route_endpoints management_endpoints_{};
    route_endpoints query_endpoints_{};
    route_endpoints search_endpoints_{};
    route_endpoints analytics_endpoints_{};
    route_endpoints eventing_endpoints_{};
    std::shared_ptr<vbucket_map> vb_map_{};
    std::shared_ptr<ketama_continuum> ketama_map_{};
    cluster_capabilities cluster_capabilities_{};
    bucket_capabilities bucket_capabilities_{};
};
} // namespace couchbase::new_core
