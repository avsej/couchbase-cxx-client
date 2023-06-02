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

#include "watch_query_indexes.hxx"

#include "core/logger/logger.hxx"
#include "core/operations/management/query_index_get_all.hxx"

namespace couchbase::core::impl
{
namespace
{
template<typename Response>
manager_error_context
build_context(Response& resp, std::optional<std::error_code> ec = {})
{
    return { ec.value_or(resp.ctx.ec), resp.ctx.last_dispatched_to,       resp.ctx.last_dispatched_from,
             resp.ctx.retry_attempts,  std::move(resp.ctx.retry_reasons), std::move(resp.ctx.client_context_id),
             resp.ctx.http_status,     std::move(resp.ctx.http_body),     std::move(resp.ctx.path) };
}

class watch_context : public std::enable_shared_from_this<watch_context>
{

  public:
    watch_context(couchbase::core::cluster core,
                  std::string bucket_name,
                  std::vector<std::string> index_names,
                  couchbase::watch_query_indexes_options::built options,
                  query_context query_ctx,
                  std::string collection_name,
                  watch_query_indexes_handler&& handler)
      : core_(std::move(core))
      , timer_{ core_.io_context() }
      , options_(std::move(options))
      , timeout_{ options_.timeout.value_or(core_.origin().second.options().query_timeout) }
      , bucket_name_(std::move(bucket_name))
      , index_names_(std::move(index_names))
      , query_ctx_(std::move(query_ctx))
      , collection_name_(std::move(collection_name))
      , handler_(std::move(handler))
    {
    }

    watch_context(watch_context&& other)
      : core_(std::move(other.core_))
      , timer_(std::move(other.timer_))
      , options_(std::move(other.options_))
      , timeout_(other.timeout_)
      , bucket_name_(std::move(other.bucket_name_))
      , index_names_(std::move(other.index_names_))
      , query_ctx_(std::move(other.query_ctx_))
      , collection_name_(std::move(other.collection_name_))
      , handler_(std::move(other.handler_))
      , start_time_(other.start_time_)
      , attempts_(other.attempts_.load())
    {
    }

    void execute()
    {
        CB_LOG_TRACE("watch indexes executing request");
        core_.execute(
          operations::management::query_index_get_all_request{ bucket_name_, "", collection_name_, query_ctx_, {}, remaining() },
          [ctx = shared_from_this()](auto&& resp) {
              CB_LOG_TRACE("watch indexes got {}", resp.ctx.ec.message());
              if (!ctx->check(resp)) {
                  // now we try again
                  ctx->poll();
              }
          });
    }

  private:
    void finish(manager_error_context error_ctx)
    {
        handler_(std::move(error_ctx));
        timer_.cancel();
    }

    std::chrono::milliseconds remaining() const
    {
        return timeout_ - std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time_);
    }

    bool check(couchbase::core::operations::management::query_index_get_all_response resp)
    {
        bool complete = true;
        for (const auto& name : index_names_) {
            auto it = std::find_if(resp.indexes.begin(), resp.indexes.end(), [&](const auto& index) { return index.name == name; });
            complete &= (it != resp.indexes.end() && it->state == "online");
        }
        if (options_.watch_primary) {
            auto it = std::find_if(resp.indexes.begin(), resp.indexes.end(), [&](const auto& index) { return index.is_primary; });
            complete &= it != resp.indexes.end() && it->state == "online";
        }
        if (complete || resp.ctx.ec == couchbase::errc::common::ambiguous_timeout) {
            finish(build_context(resp));
        } else if (remaining().count() <= 0) {
            finish(build_context(resp, couchbase::errc::common::ambiguous_timeout));
            complete = true;
        }
        return complete;
    }

    void poll()
    {
        timer_.expires_after(options_.polling_interval);
        timer_.async_wait([ctx = shared_from_this()](asio::error_code ec) {
            if (ec == asio::error::operation_aborted) {
                return;
            }
            ctx->execute();
        });
    }

    couchbase::core::cluster core_;
    asio::steady_timer timer_;
    couchbase::watch_query_indexes_options::built options_;
    std::chrono::milliseconds timeout_;
    std::string bucket_name_;
    std::vector<std::string> index_names_;
    query_context query_ctx_;
    std::string collection_name_;
    watch_query_indexes_handler handler_;
    std::chrono::steady_clock::time_point start_time_{ std::chrono::steady_clock::now() };
    std::atomic<size_t> attempts_{ 0 };
};
} // namespace

void
initiate_watch_query_indexes(couchbase::core::cluster core,
                             std::string bucket_name,
                             std::vector<std::string> index_names,
                             couchbase::watch_query_indexes_options::built options,
                             query_context query_ctx,
                             std::string collection_name,
                             watch_query_indexes_handler&& handler)
{
    auto ctx = std::make_shared<watch_context>(std::move(core),
                                               std::move(bucket_name),
                                               std::move(index_names),
                                               std::move(options),
                                               std::move(query_ctx),
                                               std::move(collection_name),
                                               std::move(handler));
    return ctx->execute();
}

void
initiate_watch_query_indexes(couchbase::core::cluster core,
                             std::string bucket_name,
                             std::vector<std::string> index_names,
                             couchbase::watch_query_indexes_options::built options,
                             watch_query_indexes_handler&& handler)
{
    return initiate_watch_query_indexes(
      std::move(core), std::move(bucket_name), std::move(index_names), std::move(options), {}, "", std::move(handler));
}

} // namespace couchbase::core::impl
