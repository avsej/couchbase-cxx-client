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

#include "route_config.hxx"

#include <fmt/core.h>

#include <utility>

namespace couchbase::new_core
{
route_config::route_config(config::bucket_type type,
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
                           bucket_capabilities bucket_caps)
  : bucket_type_(type)
  , rev_epoch_(rev_epoch)
  , rev_id_(rev_id)
  , uuid_(std::move(uuid))
  , name_(std::move(name))
  , key_value_endpoints_(std::move(key_value_endpoints))
  , views_endpoints_(std::move(views_endpoints))
  , management_endpoints_(std::move(management_endpoints))
  , query_endpoints_(std::move(query_endpoints))
  , search_endpoints_(std::move(search_endpoints))
  , analytics_endpoints_(std::move(analytics_endpoints))
  , eventing_endpoints_(std::move(eventing_endpoints))
  , cluster_capabilities_(std::move(cluster_caps))
  , bucket_capabilities_(std::move(bucket_caps))
{
}

route_config::route_config(bucket_type type,
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
                           std::shared_ptr<vbucket_map> vb_map)
  : route_config(type,
                 rev_id,
                 rev_epoch,
                 std::move(uuid),
                 std::move(name),
                 std::move(key_value_endpoints),
                 std::move(views_endpoints),
                 std::move(management_endpoints),
                 std::move(query_endpoints),
                 std::move(search_endpoints),
                 std::move(analytics_endpoints),
                 std::move(eventing_endpoints),
                 std::move(cluster_caps),
                 std::move(bucket_caps))
{
    vb_map_ = std::move(vb_map);
}

route_config::route_config(bucket_type type,
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
                           std::shared_ptr<ketama_continuum> ketama_map)
  : route_config(type,
                 rev_id,
                 rev_epoch,
                 std::move(uuid),
                 std::move(name),
                 std::move(key_value_endpoints),
                 std::move(views_endpoints),
                 std::move(management_endpoints),
                 std::move(query_endpoints),
                 std::move(search_endpoints),
                 std::move(analytics_endpoints),
                 std::move(eventing_endpoints),
                 std::move(cluster_caps),
                 std::move(bucket_caps))
{
    ketama_map_ = std::move(ketama_map);
}

std::string
route_config::debug_string() const
{
    std::vector<char> out;

    fmt::format_to(std::back_insert_iterator(out), "revision ID: {}, revision epoch: {}", rev_id_, rev_epoch_);

    if (!name_.empty()) {
        fmt::format_to(std::back_insert_iterator(out), ", bucket: \"{}\"", name_);
    }
    fmt::format_to(std::back_insert_iterator(out), "\n");

    auto add_endpoints = [&out](std::string_view title, const route_endpoints& endpoints) {
        fmt::format_to(std::back_insert_iterator(out), "{} endpoints:\n", title);
        fmt::format_to(std::back_insert_iterator(out), "  TLS:\n");
        for (const auto& ep : endpoints.ssl_endpoints) {
            fmt::format_to(std::back_insert_iterator(out), "  - {} seed: {}\n", ep.address, ep.is_seed_node);
        }
        fmt::format_to(std::back_insert_iterator(out), "  non-TLS:\n");
        for (const auto& ep : endpoints.non_ssl_endpoints) {
            fmt::format_to(std::back_insert_iterator(out), "  - {} seed: {}\n", ep.address, ep.is_seed_node);
        }
    };

    add_endpoints("views", views_endpoints_);
    add_endpoints("management", management_endpoints_);
    add_endpoints("query", query_endpoints_);
    add_endpoints("search", search_endpoints_);
    add_endpoints("analytics", analytics_endpoints_);
    add_endpoints("eventing", eventing_endpoints_);

    if (vb_map_) {
        fmt::format_to(std::back_insert_iterator(out), vb_map_->debug_string());
    } else {
        fmt::format_to(std::back_insert_iterator(out), "vBucket map: not-used");
    }
    fmt::format_to(std::back_insert_iterator(out), "\n");

    if (ketama_map_) {
        fmt::format_to(std::back_insert_iterator(out), ketama_map_->debug_string());
    } else {
        fmt::format_to(std::back_insert_iterator(out), "ketama map: not-used");
    }

    return { out.begin(), out.end() };
}

bool
route_config::is_valid() const
{
    if ((key_value_endpoints_.ssl_endpoints.empty() || management_endpoints_.ssl_endpoints.empty()) &&
        (key_value_endpoints_.non_ssl_endpoints.empty() || management_endpoints_.non_ssl_endpoints.empty())) {
        return false;
    }
    switch (bucket_type_) {
        case bucket_type::none:
            return true;
        case bucket_type::couchbase:
            return vb_map_ != nullptr && vb_map_->is_valid();
        case bucket_type::memcached:
            return ketama_map_ != nullptr && ketama_map_->is_valid();
        default:
            break;
    }
    return false;
}

bool
route_config::is_gcccp_config() const
{
    return bucket_type_ == bucket_type::none;
}

bool
route_config::is_newer_than(const route_config& other) const
{
    if (rev_epoch_ < other.rev_epoch_) {
        // other configuration has an older revision epoch
        return false;
    }
    if (rev_epoch_ == other.rev_epoch_) {
        if (rev_id_ == 0) {
            // this configuration is not versioned, switch to other config
        } else if (rev_id_ == other.rev_id_) {
            // other configuration has identical revision number
            return false;
        } else if (rev_id_ < other.rev_id_) {
            // other configuration has older revision number
            return false;
        }
    }
    return true;
}

const route_endpoints&
route_config::key_value_endpoints() const
{
    return key_value_endpoints_;
}

const route_endpoints&
route_config::management_endpoints() const
{
    return management_endpoints_;
}

std::int64_t
route_config::rev_id() const
{
    return rev_id_;
}

std::shared_ptr<vbucket_map>
route_config::vb_map() const
{
    return vb_map_;
}

bool
route_config::has_vbucket_map() const
{
    return vb_map_ != nullptr;
}

bucket_type
route_config::type() const
{
    return bucket_type_;
}

auto
route_config::bucket_caps() const -> bucket_capabilities
{
    return bucket_capabilities_;
}

auto
route_config::cluster_caps() const -> cluster_capabilities
{
    return cluster_capabilities_
}

auto
route_config::ketama_map() const -> std::shared_ptr<ketama_continuum>
{
    return ketama_map_;
}
auto
route_config::name() const -> const std::string&
{
    return name_;
}
} // namespace couchbase::new_core
