/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2020-Present Couchbase, Inc.
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

#include "core/cluster.hxx"
#include "core/operations/management/query_index_build_deferred.hxx"
#include "core/operations/management/query_index_create.hxx"
#include "core/operations/management/query_index_drop.hxx"
#include "core/operations/management/query_index_get_all.hxx"

#include "watch_query_indexes.hxx"

#include <couchbase/query_index_manager.hxx>

namespace couchbase
{
namespace
{
template<typename Response>
manager_error_context
build_context(Response& resp)
{
    return { resp.ctx.ec,
             resp.ctx.last_dispatched_to,
             resp.ctx.last_dispatched_from,
             resp.ctx.retry_attempts,
             std::move(resp.ctx.retry_reasons),
             std::move(resp.ctx.client_context_id),
             resp.ctx.http_status,
             std::move(resp.ctx.http_body),
             std::move(resp.ctx.path) };
}

} // namespace

class query_index_manager_impl : public std::enable_shared_from_this<query_index_manager_impl>
{
  public:
    explicit query_index_manager_impl(core::cluster core)
      : core_{ std::move(core) }
    {
    }

    void get_all_indexes(const std::string& bucket_name,
                         get_all_query_indexes_options::built options,
                         get_all_query_indexes_handler&& handler)
    {
        core_.execute(
          core::operations::management::query_index_get_all_request{
            bucket_name,
            {},
            {},
            {},
            {},
            options.timeout,
          },
          [handler = std::move(handler)](core::operations::management::query_index_get_all_response resp) {
              if (resp.ctx.ec) {
                  return handler(build_context(resp), {});
              }
              handler(build_context(resp), resp.indexes);
          });
    }

    void create_index(const std::string& bucket_name,
                      std::string index_name,
                      std::vector<std::string> fields,
                      create_query_index_options::built options,
                      create_query_index_handler&& handler)
    {
        core_.execute(
          core::operations::management::query_index_create_request{
            bucket_name,
            {},
            {},
            std::move(index_name),
            std::move(fields),
            {},
            false /* is_primary */,
            options.ignore_if_exists,
            options.condition,
            options.deferred,
            options.num_replicas,
            {},
            options.timeout,
          },
          [handler = std::move(handler)](auto&& resp) { handler(build_context(resp)); });
    }

    void create_primary_index(const std::string& bucket_name,
                              create_primary_query_index_options::built options,
                              create_primary_query_index_handler&& handler)
    {
        core_.execute(
          core::operations::management::query_index_create_request{
            bucket_name,
            {},
            {},
            options.index_name.value_or(""),
            {},
            {},
            true /* is_primary */,
            options.ignore_if_exists,
            {},
            options.deferred,
            options.num_replicas,
            {},
            options.timeout,
          },
          [handler = std::move(handler)](auto&& resp) { handler(build_context(resp)); });
    }

    void drop_index(const std::string& bucket_name,
                    std::string index_name,
                    drop_query_index_options::built options,
                    drop_query_index_handler&& handler)
    {
        core_.execute(
          core::operations::management::query_index_drop_request{
            bucket_name,
            {},
            {},
            std::move(index_name),
            {},
            false /* is_primary */,
            options.ignore_if_not_exists,
            {},
            options.timeout,
          },
          [handler = std::move(handler)](auto&& resp) { handler(build_context(resp)); });
    }

    void drop_primary_index(const std::string& bucket_name,
                            drop_primary_query_index_options::built options,
                            drop_primary_query_index_handler&& handler)
    {
        core_.execute(
          core::operations::management::query_index_drop_request{
            bucket_name,
            {},
            {},
            options.index_name.value_or(""),
            {},
            true,
            options.ignore_if_not_exists,
            {},
            options.timeout,
          },
          [handler = std::move(handler)](core::operations::management::query_index_drop_response resp) { handler(build_context(resp)); });
    }

    void build_deferred_indexes(const std::string& bucket_name,
                                build_query_index_options::built options,
                                build_deferred_query_indexes_handler&& handler)
    {
        auto timeout = options.timeout;
        core_.execute(
          core::operations::management::query_index_get_all_deferred_request{ bucket_name, {}, {}, {}, {}, timeout },
          [self = shared_from_this(), bucket_name, options = std::move(options), handler = std::move(handler)](auto&& list_resp) mutable {
              if (list_resp.ctx.ec) {
                  return handler(build_context(list_resp));
              }
              if (list_resp.index_names.empty()) {
                  return handler(build_context(list_resp));
              }
              self->core_.execute(
                core::operations::management::query_index_build_request{
                  bucket_name, {}, {}, {}, std::move(list_resp.index_names), {}, options.timeout },
                [handler = std::move(handler)](auto&& build_resp) { return handler(build_context(build_resp)); });
          });
    }

    void watch_indexes(std::string bucket_name,
                       std::vector<std::string> index_names,
                       watch_query_indexes_options::built options,
                       watch_query_indexes_handler&& handler)
    {

        core::impl::initiate_watch_query_indexes(
          core_, std::move(bucket_name), std::move(index_names), std::move(options), {}, {}, std::move(handler));
    }

  private:
    core::cluster core_;
};

query_index_manager::query_index_manager(core::cluster core)
  : impl_(std::make_shared<query_index_manager_impl>(std::move(core)))
{
}

void
query_index_manager::get_all_indexes(std::string bucket_name,
                                     const get_all_query_indexes_options& options,
                                     get_all_query_indexes_handler&& handler) const
{
    return impl_->get_all_indexes(bucket_name, options.build(), std::forward<get_all_query_indexes_handler>(handler));
}

auto
query_index_manager::get_all_indexes(std::string bucket_name, const get_all_query_indexes_options& options) const
  -> std::future<std::pair<manager_error_context, std::vector<management::query_index>>>
{
    auto barrier = std::make_shared<std::promise<std::pair<manager_error_context, std::vector<management::query_index>>>>();
    auto future = barrier->get_future();
    get_all_indexes(std::move(bucket_name), options, [barrier](auto ctx, auto resp) { barrier->set_value({ ctx, resp }); });
    return future;
}

void
query_index_manager::create_index(std::string bucket_name,
                                  std::string index_name,
                                  std::vector<std::string> fields,
                                  const create_query_index_options& options,
                                  create_query_index_handler&& handler) const
{
    return impl_->create_index(bucket_name, std::move(index_name), std::move(fields), options.build(), std::move(handler));
}

auto
query_index_manager::create_index(std::string bucket_name,
                                  std::string index_name,
                                  std::vector<std::string> fields,
                                  const create_query_index_options& options) const -> std::future<manager_error_context>
{
    auto barrier = std::make_shared<std::promise<manager_error_context>>();
    auto future = barrier->get_future();
    create_index(
      std::move(bucket_name), std::move(index_name), std::move(fields), options, [barrier](auto ctx) { barrier->set_value(ctx); });
    return future;
}

void
query_index_manager::create_primary_index(std::string bucket_name,
                                          const create_primary_query_index_options& options,
                                          create_query_index_handler&& handler) const
{
    return impl_->create_primary_index(bucket_name, options.build(), std::move(handler));
}

auto
query_index_manager::create_primary_index(std::string bucket_name, const create_primary_query_index_options& options) const
  -> std::future<manager_error_context>
{
    auto barrier = std::make_shared<std::promise<manager_error_context>>();
    auto future = barrier->get_future();
    create_primary_index(std::move(bucket_name), options, [barrier](auto ctx) { barrier->set_value(ctx); });
    return future;
}

void
query_index_manager::drop_primary_index(std::string bucket_name,
                                        const drop_primary_query_index_options& options,
                                        drop_query_index_handler&& handler) const
{
    return impl_->drop_primary_index(bucket_name, options.build(), std::move(handler));
}

auto
query_index_manager::drop_primary_index(std::string bucket_name, const drop_primary_query_index_options& options) const
  -> std::future<manager_error_context>
{
    auto barrier = std::make_shared<std::promise<manager_error_context>>();
    auto future = barrier->get_future();
    drop_primary_index(std::move(bucket_name), options, [barrier](auto ctx) { barrier->set_value(ctx); });
    return future;
}

void
query_index_manager::drop_index(std::string bucket_name,
                                std::string index_name,
                                const drop_query_index_options& options,
                                drop_query_index_handler&& handler) const
{
    return impl_->drop_index(bucket_name, std::move(index_name), options.build(), std::move(handler));
}

auto
query_index_manager::drop_index(std::string bucket_name, std::string index_name, const drop_query_index_options& options) const
  -> std::future<manager_error_context>
{
    auto barrier = std::make_shared<std::promise<manager_error_context>>();
    auto future = barrier->get_future();
    drop_index(std::move(bucket_name), std::move(index_name), options, [barrier](auto ctx) { barrier->set_value(ctx); });
    return future;
}

void
query_index_manager::build_deferred_indexes(std::string bucket_name,
                                            const build_query_index_options& options,
                                            build_deferred_query_indexes_handler&& handler) const
{
    return impl_->build_deferred_indexes(bucket_name, options.build(), std::forward<build_deferred_query_indexes_handler>(handler));
}
auto
query_index_manager::build_deferred_indexes(std::string bucket_name, const build_query_index_options& options) const
  -> std::future<manager_error_context>
{
    auto barrier = std::make_shared<std::promise<manager_error_context>>();
    auto future = barrier->get_future();
    build_deferred_indexes(std::move(bucket_name), options, [barrier](auto ctx) { barrier->set_value(std::move(ctx)); });
    return future;
}

void
query_index_manager::watch_indexes(std::string bucket_name,
                                   std::vector<std::string> index_names,
                                   const watch_query_indexes_options& options,
                                   watch_query_indexes_handler&& handler) const
{
    return impl_->watch_indexes(bucket_name, std::move(index_names), options.build(), std::move(handler));
}

auto
query_index_manager::watch_indexes(std::string bucket_name,
                                   std::vector<std::string> index_names,
                                   const watch_query_indexes_options& options) const -> std::future<manager_error_context>
{
    auto barrier = std::make_shared<std::promise<manager_error_context>>();
    auto future = barrier->get_future();
    watch_indexes(std::move(bucket_name), std::move(index_names), options, [barrier](auto ctx) { barrier->set_value(ctx); });
    return future;
}
} // namespace couchbase
