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

#include "core/config_listener.hxx"

#include <memory>
#include <string>

namespace asio
{
class io_context;
namespace ssl
{
class context;
} // namespace ssl
} // namespace asio

namespace couchbase
{
namespace tracing
{
class request_tracer;
} // namespace tracing
namespace metrics
{
class meter;
} // namespace metrics

namespace core
{
struct cluster_options;

namespace diag
{
struct diagnostics_result;
}

namespace io
{
class http_session_manager_impl;

class http_session_manager : public config_listener
{
  public:
    http_session_manager(std::string client_id, asio::io_context& ctx, asio::ssl::context& tls);

    void update_config(const topology::configuration& config) override;

    void set_tracer(std::shared_ptr<couchbase::tracing::request_tracer> tracer);
    void set_meter(std::shared_ptr<couchbase::metrics::meter> meter);
    void set_configuration(const topology::configuration& config, const cluster_options& options);
    void close();
    void export_diag_info(diag::diagnostics_result& res);

  private:
    std::shared_ptr<http_session_manager_impl> impl_;
};

} // namespace io
} // namespace core
} // namespace couchbase
