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

#include "config_management_component.hxx"

#include "address_utils.hxx"

#include "core/logger/logger.hxx"
#include "core/utils/json.hxx"

#include <algorithm>

namespace couchbase::new_core::impl
{
struct server_endpoints {
    route_endpoint key_value_ssl;
    route_endpoint key_value;
    route_endpoint views_ssl;
    route_endpoint views;
    route_endpoint management_ssl;
    route_endpoint management;
    route_endpoint query_ssl;
    route_endpoint query;
    route_endpoint search_ssl;
    route_endpoint search;
    route_endpoint analytics_ssl;
    route_endpoint analytics;
    route_endpoint eventing_ssl;
    route_endpoint eventing;
};

[[nodiscard]] static server_endpoints
endpoints_from_ports(const tao::json::value& ports, const std::string& hostname, bool is_seed_node)
{
    server_endpoints lists{};
    if (const auto* service = ports.find("kvSSL"); service != nullptr && service->is_number()) {
        lists.key_value_ssl = { fmt::format("couchbases://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("capiSSL"); service != nullptr && service->is_number()) {
        lists.views_ssl = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("mgmtSSL"); service != nullptr && service->is_number()) {
        lists.management_ssl = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("n1qlSSL"); service != nullptr && service->is_number()) {
        lists.query_ssl = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("ftsSSL"); service != nullptr && service->is_number()) {
        lists.search_ssl = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("cbasSSL"); service != nullptr && service->is_number()) {
        lists.analytics_ssl = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("eventingSSL"); service != nullptr && service->is_number()) {
        lists.eventing_ssl = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("kv"); service != nullptr && service->is_number()) {
        lists.key_value = { fmt::format("couchbase://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("capi"); service != nullptr && service->is_number()) {
        lists.views = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("mgmt"); service != nullptr && service->is_number()) {
        lists.management = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("n1ql"); service != nullptr && service->is_number()) {
        lists.query = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("fts"); service != nullptr && service->is_number()) {
        lists.search = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("cbas"); service != nullptr && service->is_number()) {
        lists.analytics = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    if (const auto* service = ports.find("eventingAdminPort"); service != nullptr && service->is_number()) {
        lists.eventing = { fmt::format("https://{}:{}", hostname, service->get_unsigned()), is_seed_node };
    }
    return lists;
}

[[nodiscard]] static std::string
trim_scheme_prefix(std::string address)
{
    constexpr std::string_view schema_separator{ "://" };
    auto idx = address.find(schema_separator);
    if (idx != std::string::npos) {
        return address;
    }
    return address.substr(idx + schema_separator.size());
}

[[nodiscard]] static std::string
get_hostname(const std::string& hostname, const std::string& source_hostname)
{
    // hostname blank means to use the same one as was connected to
    if (hostname.empty()) {
        // note: source_hostname will already be IPv6 wrapped
        return source_hostname;
    }
    // we need to detect and IPv6 address here and wrap it in the appropriate [] block to indicate its IPv6 for the rest of the system.
    if (hostname.find(':') != std::string::npos) {
        return fmt::format("[{}]", hostname);
    }
    return hostname;
}

[[nodiscard]] static std::uint64_t
get_direct_port(const tao::json::value& node)
{
    const auto* ports = node.find("ports");
    if (ports == nullptr || !ports->is_object()) {
        return 0;
    }
    const auto* direct = ports->find("direct");
    if (direct == nullptr || !direct->is_number()) {
        return 0;
    }
    return direct->get_unsigned();
}

[[nodiscard]] static std::shared_ptr<vbucket_map>
get_vb_map(const tao::json::value& config)
{
    if (const auto* vbucket_server_map = config.find("vBucketServerMap");
        vbucket_server_map != nullptr && vbucket_server_map->is_object()) {
        if (const auto* map = vbucket_server_map->find("vBucketMap"); map != nullptr && map->is_array()) {
            const auto& vbm = map->get_array();
            std::vector<std::vector<int>> entries;
            entries.resize(vbm.size());
            for (std::size_t i = 0; i < vbm.size(); ++i) {
                const auto& vb = vbm[i].get_array();
                entries[i].resize(vb.size());
                for (std::size_t n = 0; n < vb.size(); n++) {
                    entries[i][n] = vb[n].as<std::int16_t>();
                }
            }
            auto num_replicas = vbucket_server_map->optional<std::uint32_t>("numReplicas").value_or(0);
            return std::make_shared<vbucket_map>(entries, num_replicas);
        }
    }
    return {};
}

[[nodiscard]] static cluster_capabilities
get_cluster_capabilities(const tao::json::value& config)
{
    if (const auto* ver = config.find("clusterCapabilitiesVer"); ver != nullptr && ver->is_array()) {
        std::vector<std::uint32_t> versions{};
        for (const auto& num : ver->get_array()) {
            if (!num.is_number()) {
                continue;
            }
            versions.emplace_back(num.get_unsigned());
        }

        if (versions.front() == 1) {
            if (const auto* caps = config.find("clusterCapabilities"); caps != nullptr && caps->is_object()) {
                std::set<cluster_capability> supported_caps;
                if (const auto* n1ql = caps->find("n1ql"); n1ql != nullptr && n1ql->is_array()) {
                    for (const auto& entry : n1ql->get_array()) {
                        if (!entry.is_string()) {
                            continue;
                        }
                        if (const auto& name = entry.get_string(); name == "costBasedOptimizer") {
                            supported_caps.insert(cluster_capability::n1ql_cost_based_optimizer);
                        } else if (name == "indexAdvisor") {
                            supported_caps.insert(cluster_capability::n1ql_index_advisor);
                        } else if (name == "javaScriptFunctions") {
                            supported_caps.insert(cluster_capability::n1ql_javascript_functions);
                        } else if (name == "inlineFunctions") {
                            supported_caps.insert(cluster_capability::n1ql_inline_functions);
                        } else if (name == "enhancedPreparedStatements") {
                            supported_caps.insert(cluster_capability::n1ql_enhanced_prepared_statements);
                        }
                    }
                }
                return cluster_capabilities{ versions, supported_caps };
            }
        }
        return cluster_capabilities{ versions, {} };
    }

    return {};
}

[[nodiscard]] static bucket_capabilities
get_bucket_capabilities(const tao::json::value& config)
{
    if (const auto* caps = config.find("bucketCapabilities"); caps != nullptr && caps->is_array()) {
        std::set<bucket_capability> supported_caps;
        for (const auto& entry : caps->get_array()) {
            if (!entry.is_string()) {
                continue;
            }
            if (const auto& name = entry.get_string(); name == "couchapi") {
                supported_caps.insert(bucket_capability::couchapi);
            } else if (name == "collections") {
                supported_caps.insert(bucket_capability::collections);
            } else if (name == "durableWrite") {
                supported_caps.insert(bucket_capability::durable_write);
            } else if (name == "tombstonedUserXAttrs") {
                supported_caps.insert(bucket_capability::tombstoned_user_xattrs);
            } else if (name == "dcp") {
                supported_caps.insert(bucket_capability::dcp);
            } else if (name == "cbhello") {
                supported_caps.insert(bucket_capability::cbhello);
            } else if (name == "touch") {
                supported_caps.insert(bucket_capability::touch);
            } else if (name == "cccp") {
                supported_caps.insert(bucket_capability::cccp);
            } else if (name == "xdcrCheckpointing") {
                supported_caps.insert(bucket_capability::xdcr_checkpointing);
            } else if (name == "nodesExt") {
                supported_caps.insert(bucket_capability::nodes_ext);
            } else if (name == "xattr") {
                supported_caps.insert(bucket_capability::xattr);
            } else if (name == "preserveExpiry") {
                supported_caps.insert(bucket_capability::preserve_expiry);
            } else if (name == "rangeScan") {
                supported_caps.insert(bucket_capability::range_scan);
            } else if (name == "subdoc.DocumentMacroSupport") {
                supported_caps.insert(bucket_capability::subdoc_document_macro_support);
            } else if (name == "subdoc.ReplaceBodyWithXattr") {
                supported_caps.insert(bucket_capability::subdoc_replace_body_with_xattr);
            } else if (name == "subdoc.ReviveDocument") {
                supported_caps.insert(bucket_capability::subdoc_revive_document);
            }
        }
        return bucket_capabilities{ supported_caps };
    }

    return {};
}

/**
 * builds a new route config from this config.
 *
 * overwrite_seed_node indicates that we should set the hostname for a node to the source_hostname when the config has been sourced
 * from that node.
 */
[[nodiscard]] static std::shared_ptr<route_config>
build_route_config(const config_value& input, bool use_ssl, const std::string& network_type, bool first_connect, bool overwrite_seed_node)
{
    bucket_type bkt_type{ bucket_type::invalid };
    if (const auto* node_locator = input.config.find("nodeLocator"); node_locator != nullptr && node_locator->is_string()) {
        if (const auto& locator = node_locator->get_string(); locator == "ketama") {
            bkt_type = bucket_type::memcached;
        } else if (locator == "vbucket") {
            bkt_type = bucket_type::couchbase;
        } else {
            if (const auto* uuid = input.config.find("uuid"); uuid == nullptr || (uuid->is_string() && uuid->get_string().empty())) {
                bkt_type = bucket_type::none;
            } else {
                LOG_DEBUG("invalid nodeLocator: {}", locator);
                bkt_type = bucket_type::invalid;
            }
        }
    }

    route_endpoints key_value_endpoints;
    route_endpoints views_endpoints;
    route_endpoints management_endpoints;
    route_endpoints query_endpoints;
    route_endpoints search_endpoints;
    route_endpoints analytics_endpoints;
    route_endpoints eventing_endpoints;

    if (const auto* nodes_ext = input.config.find("nodesExt"); nodes_ext != nullptr && nodes_ext->is_array()) {
        std::size_t nodes_size = input.config.at("nodes").get_array().size();
        const auto& nodes = nodes_ext->get_array();
        for (std::size_t index = 0; index < nodes.size(); ++index) {
            const auto& node = nodes[index];
            std::string hostname = node.at("hostname").get_string();
            const auto* ports = node.find("services");
            if (network_type != "default") {
                if (const auto* alt = node.find("alternateAddresses"); alt != nullptr && alt->is_object()) {
                    if (const auto* addr = alt->find(network_type); addr != nullptr && addr->is_object()) {
                        if (const auto* h = addr->find("hostname"); h != nullptr && h->is_string()) {
                            hostname = h->get_string();
                        }
                        if (const auto* p = addr->find("ports"); p != nullptr && p->is_object()) {
                            ports = p;
                        }
                    } else {
                        if (!first_connect) {
                            LOG_DEBUG("invalid config network type \"{}\"", network_type);
                        }
                        continue;
                    }
                }
            }

            bool is_seed_node = (node.find("thisNode") != nullptr && node.is_boolean() && node.get_boolean()) || hostname.empty();

            if (is_seed_node && overwrite_seed_node) {
                LOG_TRACE("seed node detected and set to overwrite, setting hostname to {}", input.source_host);
                hostname = input.source_host;
            } else {
                hostname = get_hostname(hostname, input.source_host);
            }

            auto endpoints = endpoints_from_ports(*ports, hostname, is_seed_node);
            if (!endpoints.key_value.address.empty()) {
                if (bkt_type != bucket_type::invalid && bkt_type != bucket_type::none && index >= nodes_size) {
                    LOG_DEBUG("KV node present in nodesExt, but not in nodes for {}", endpoints.key_value.address);
                } else {
                    key_value_endpoints.non_ssl_endpoints.emplace_back(std::move(endpoints.key_value));
                }
            }
            if (!endpoints.views.address.empty()) {
                views_endpoints.non_ssl_endpoints.emplace_back(endpoints.views);
            }
            if (!endpoints.management.address.empty()) {
                management_endpoints.non_ssl_endpoints.emplace_back(endpoints.management);
            }
            if (!endpoints.query.address.empty()) {
                query_endpoints.non_ssl_endpoints.emplace_back(endpoints.query);
            }
            if (!endpoints.search.address.empty()) {
                search_endpoints.non_ssl_endpoints.emplace_back(endpoints.search);
            }
            if (!endpoints.analytics.address.empty()) {
                analytics_endpoints.non_ssl_endpoints.emplace_back(endpoints.analytics);
            }
            if (!endpoints.eventing.address.empty()) {
                eventing_endpoints.non_ssl_endpoints.emplace_back(endpoints.eventing);
            }

            if (!endpoints.key_value_ssl.address.empty()) {
                if (bkt_type != bucket_type::invalid && bkt_type != bucket_type::none && index >= nodes_size) {
                    LOG_DEBUG("KV node present in nodesExt, but not in nodes for {}", endpoints.key_value_ssl.address);
                } else {
                    key_value_endpoints.ssl_endpoints.emplace_back(std::move(endpoints.key_value_ssl));
                }
            }
            if (!endpoints.views_ssl.address.empty()) {
                views_endpoints.ssl_endpoints.emplace_back(endpoints.views);
            }
            if (!endpoints.management_ssl.address.empty()) {
                management_endpoints.ssl_endpoints.emplace_back(endpoints.management);
            }
            if (!endpoints.query_ssl.address.empty()) {
                query_endpoints.ssl_endpoints.emplace_back(endpoints.query);
            }
            if (!endpoints.search_ssl.address.empty()) {
                search_endpoints.ssl_endpoints.emplace_back(endpoints.search);
            }
            if (!endpoints.analytics_ssl.address.empty()) {
                analytics_endpoints.ssl_endpoints.emplace_back(endpoints.analytics);
            }
            if (!endpoints.eventing_ssl.address.empty()) {
                eventing_endpoints.ssl_endpoints.emplace_back(endpoints.eventing);
            }
        }
    } else {
        if (use_ssl) {
            LOG_ERROR("received config without nodesExt while SSL is enabled. Generating invalid config.");
            return {};
        }

        if (bkt_type == bucket_type::couchbase) {
            if (const auto* vbucket_server_map = input.config.find("vBucketServerMap");
                vbucket_server_map != nullptr && vbucket_server_map->is_object()) {
                if (const auto* server_list = vbucket_server_map->find("serverList"); server_list != nullptr && server_list->is_array()) {
                    key_value_endpoints.non_ssl_endpoints.reserve(server_list->get_array().size());
                    for (const auto& s : server_list->get_array()) {
                        key_value_endpoints.non_ssl_endpoints.emplace_back(route_endpoint{ s.get_string(), false });
                    }
                }
            }
        }

        if (const auto* nodes = input.config.find("nodes"); nodes != nullptr && nodes->is_array()) {
            for (const auto& node : nodes->get_array()) {
                if (const auto* couch_api_base = node.find("couchApiBase"); couch_api_base != nullptr && couch_api_base->is_string()) {
                    // remove the UUID from the URL
                    std::string capi_endpoint = couch_api_base->get_string();
                    capi_endpoint.erase(capi_endpoint.find("%2B"));
                    views_endpoints.non_ssl_endpoints.emplace_back(route_endpoint{ capi_endpoint, false });
                }
                if (const auto* hostname = node.find("hostname"); hostname != nullptr && hostname->is_string()) {
                    management_endpoints.non_ssl_endpoints.emplace_back(
                      route_endpoint{ fmt::format("http://{}", hostname->get_string()), false });

                    if (bkt_type == bucket_type::memcached) {
                        // get the data port, no VBucketServerMap
                        if (auto direct_port = get_direct_port(node); direct_port > 0) {
                            key_value_endpoints.non_ssl_endpoints.emplace_back(route_endpoint{
                              fmt::format("{}:{}", utils::host_from_host_port(hostname->get_string()), direct_port), false });
                        }
                    }
                }
            }
        }
    }

    std::int64_t rev_id = input.config.optional<std::int64_t>("rev").value_or(0);
    std::int64_t rev_epoch = input.config.optional<std::int64_t>("revEpoch").value_or(0);
    std::string uuid = input.config.optional<std::string>("uuid").value_or("");
    std::string name = input.config.optional<std::string>("name").value_or("");

    cluster_capabilities cluster_caps = get_cluster_capabilities(input.config);
    bucket_capabilities bucket_caps = get_bucket_capabilities(input.config);

    if (bkt_type == bucket_type::couchbase) {
        return std::make_shared<route_config>(bkt_type,
                                              rev_id,
                                              rev_epoch,
                                              uuid,
                                              name,
                                              key_value_endpoints,
                                              views_endpoints,
                                              management_endpoints,
                                              query_endpoints,
                                              search_endpoints,
                                              analytics_endpoints,
                                              eventing_endpoints,
                                              cluster_caps,
                                              bucket_caps,
                                              get_vb_map(input.config));
    }

    if (bkt_type == bucket_type::memcached) {
        return std::make_shared<route_config>(
          bkt_type,
          rev_id,
          rev_epoch,
          uuid,
          name,
          key_value_endpoints,
          views_endpoints,
          management_endpoints,
          query_endpoints,
          search_endpoints,
          analytics_endpoints,
          eventing_endpoints,
          cluster_caps,
          bucket_caps,
          std::make_shared<ketama_continuum>(use_ssl ? key_value_endpoints.ssl_endpoints : key_value_endpoints.non_ssl_endpoints));
    }

    return std::make_shared<route_config>(bkt_type,
                                          rev_id,
                                          rev_epoch,
                                          uuid,
                                          name,
                                          key_value_endpoints,
                                          views_endpoints,
                                          management_endpoints,
                                          query_endpoints,
                                          search_endpoints,
                                          analytics_endpoints,
                                          eventing_endpoints,
                                          cluster_caps,
                                          bucket_caps);
}

void
config_management_component::on_new_config(const config_value& input)
{
    std::shared_ptr<route_config> route_config{};
    bool use_ssl = use_ssl_.load();
    if (seen_config_) {
        route_config = build_route_config(input, use_ssl, network_type_, false, no_tls_seed_node_);
    } else {
        route_config = build_first_route_config(input, use_ssl);
        LOG_DEBUG("using network type \"{}\" for connections", network_type_);
    }
    if (!route_config || !route_config->is_valid()) {
        LOG_DEBUG("routing data is not valid, skipping update:\n{}", route_config->debug_string());
    }

    // there's something wrong with this route input, so don't send it to the watchers
    if (!update_route_config(route_config)) {
        return;
    }

    current_config_ = route_config;
    seen_config_ = true;

    LOG_DEBUG("sending out routing data (update)...\nnew routing data:\n{}", route_config->debug_string());

    // we can end up deadlocking if we iterate whilst in the lock and a watcher decides to remove itself

    std::vector<std::shared_ptr<route_config_watcher>> watchers;
    {
        std::scoped_lock lock(watchers_mutex_);
        watchers = config_change_watchers_;
    }

    for (const auto& watcher : watchers) {
        watcher->on_new_route_config(current_config_);
    }
}

bool
config_management_component::update_route_config(std::shared_ptr<route_config> config)
{
    auto old_config = current_config_;

    // check some basic things to ensure consistency
    if (old_config->rev_id() > -1) {
        if ((config->has_vbucket_map() != old_config->has_vbucket_map()) ||
            (config->has_vbucket_map() && config->vb_map()->number_of_vbuckets() != old_config->vb_map()->number_of_vbuckets())) {
            LOG_WARNING("received a configuration with a different number of vbuckets, ignoring");
            return false;
        }
    }

    // check that the new config data is newer than the current one, in the case where we've done a select bucket
    // against an existing connection then the revisions could be the same. In that case the configuration still
    // needs to be applied.
    // In the case where the rev epochs are the same then we need to compare rev IDs. If the new config epoch is lower
    // than the old one then we ignore it, if it's newer then we apply the new config.

    if (config->bucket_type() != old_config->bucket_type()) {
        LOG_DEBUG("configuration data changed bucket type, switching");
        return true;
    }
    return config->is_newer_than(*old_config);
}

std::shared_ptr<route_config>
config_management_component::build_first_route_config(const config_value& config, bool use_ssl)
{
    if (!network_type_.empty() && network_type_ != "auto") {
        return build_route_config(config, use_ssl, network_type_, true, no_tls_seed_node_);
    }

    auto default_route_config = build_route_config(config, use_ssl, "default", true, no_tls_seed_node_);
    if (!default_route_config) {
        return {};
    }

    std::vector<route_endpoint> key_value_endpoints;
    std::vector<route_endpoint> management_endpoints;

    if (use_ssl) {
        key_value_endpoints = default_route_config->key_value_endpoints().ssl_endpoints;
        management_endpoints = default_route_config->management_endpoints().ssl_endpoints;
    } else {
        key_value_endpoints = default_route_config->key_value_endpoints().non_ssl_endpoints;
        management_endpoints = default_route_config->management_endpoints().non_ssl_endpoints;
    }

    // iterate over all the source servers and check if any addresses match as default or external network types
    for (const auto& src_server : source_servers_) {
        // first we check if the source server is from the defaults list
        bool src_in_default_config{ false };
        for (const auto& endpoint : key_value_endpoints) {
            if (trim_scheme_prefix(endpoint.address) == src_server.address) {
                src_in_default_config = true;
            }
        }
        for (const auto& endpoint : management_endpoints) {
            if (endpoint == src_server) {
                src_in_default_config = true;
            }
        }
        if (src_in_default_config) {
            network_type_ = "default";
            return default_route_config;
        }
    }

    // next lets se if we have and external config, if so, default to that
    if (auto external_route_config = build_route_config(config, use_ssl, "external", true, no_tls_seed_node_);
        external_route_config && external_route_config->is_valid()) {
        network_type_ = "external";
        return external_route_config;
    }

    // if all else fails, default to the implicit default config
    network_type_ = "default";
    return default_route_config;
}

void
config_management_component::remove_config_watcher(std::shared_ptr<route_config_watcher> watcher)
{
    std::scoped_lock lock(watchers_mutex_);
    config_change_watchers_.erase(std::remove(config_change_watchers_.begin(), config_change_watchers_.end(), watcher),
                                  config_change_watchers_.end());
}

void
config_management_component::add_config_watcher(std::shared_ptr<route_config_watcher> watcher)
{
    std::scoped_lock lock(watchers_mutex_);
    config_change_watchers_.emplace_back(std::move(watcher));
}
} // namespace couchbase::new_core::impl
