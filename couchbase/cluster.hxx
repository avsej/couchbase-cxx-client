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

#include <couchbase/diagnostics.hxx>
#include <couchbase/origin.hxx>

#include <asio/io_context.hpp>

#include <memory>
#include <set>

namespace couchbase
{

class cluster
{
  public:
    explicit cluster(asio::io_context& ctx);

    [[nodiscard]] static std::shared_ptr<cluster> create(asio::io_context& ctx)
    {
        return std::make_shared<cluster>(ctx);
    }

    void open(const couchbase::origin& origin, std::function<void(std::error_code)>&& handler);

    void close(std::function<void()>&& handler);

    void open_bucket(const std::string& bucket_name, std::function<void(std::error_code)>&& handler);

    void close_bucket(const std::string& bucket_name, std::function<void(std::error_code)>&& handler);

    void ping(std::optional<std::string> report_id,
              std::optional<std::string> bucket_name,
              std::set<service_type> services,
              std::function<void(couchbase::diag::ping_result)>&& handler);

    void diagnostics(std::optional<std::string> report_id, std::function<void(couchbase::diag::diagnostics_result)>&& handler);

  private:
    class impl;

    std::shared_ptr<impl> impl_;
};

template<typename Handler>
void
open(couchbase::cluster& cluster, const couchbase::origin& origin, Handler&& handler)
{
    return cluster.open(origin, std::forward<Handler>(handler));
}

template<typename Handler>
void
close(couchbase::cluster& cluster, Handler&& handler)
{
    return cluster.close(std::forward<Handler>(handler));
}

template<typename Handler>
void
open_bucket(couchbase::cluster& cluster, const std::string& bucket_name, Handler&& handler)
{
    return cluster.open_bucket(bucket_name, std::forward<Handler>(handler));
}

template<typename Handler>
void
close_bucket(couchbase::cluster& cluster, const std::string& bucket_name, Handler&& handler)
{
    return cluster.close_bucket(bucket_name, std::forward<Handler>(handler));
}

template<typename Handler>
void
ping(couchbase::cluster& cluster,
     std::optional<std::string> report_id,
     std::optional<std::string> bucket_name,
     std::set<service_type> services,
     Handler&& handler)
{
    return cluster.ping(report_id, bucket_name, services, std::forward<Handler>(handler));
}

template<typename Handler>
void
diagnostics(couchbase::cluster& cluster, std::optional<std::string> report_id, Handler&& handler)
{
    return cluster.diagnostics(report_id, std::forward<Handler>(handler));
}

} // namespace couchbase
