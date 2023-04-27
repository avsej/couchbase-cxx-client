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

#include "core_error_context.hxx"
#include "core_view_error_context.hxx"

#include <couchbase/fmt/retry_reason.hxx>

#include <tao/json/to_string.hpp>

#include <gsl/assert>

namespace couchbase
{

class core_error_context_impl
{
  public:
    core_error_context_impl(core_operation_info info, tao::json::value ctx)
      : info_{ std::move(info) }
      , ctx_{ std::move(ctx) }
    {
    }

    void merge(tao::json::value&& extra)
    {
        if (extra.is_object()) {
            ctx_.get_object().merge(extra.get_object());
        }
    }

    [[nodiscard]] auto info() const -> const core_operation_info&
    {
        return info_;
    }

    [[nodiscard]] auto to_json_string() const -> std::string
    {
        tao::json::value info = tao::json::empty_object;
        info["ec"] = tao::json::value{
            { "code", info_.ec_.value() },
            { "message", info_.ec_.message() },
        };
        if (info_.last_dispatched_to_) {
            info["last_dispatched_to"] = info_.last_dispatched_to_.value();
        }
        if (info_.last_dispatched_from_) {
            info["last_dispatched_from"] = info_.last_dispatched_from_.value();
        }
        info["retry_attempts"] = info_.retry_attempts_;
        if (!info_.retry_reasons_.empty()) {
            tao::json::value reasons = tao::json::empty_array;
            for (const auto& reason : info_.retry_reasons_) {
                reasons.emplace_string(fmt::format("{}", reason));
            }
            info["retry_reasons"] = reasons;
        }

        tao::json::value result = ctx_;
        result["info"] = info;
        return tao::json::to_string(result);
    }

    [[nodiscard]] auto get_vector_of_string(std::string_view key) const -> std::vector<std::string>
    {
        static const std::vector<std::string> empty_vector;
        if (const auto* value = ctx_.find(key); value != nullptr && value->is_array()) {
            const auto& vector = value->get_array();
            std::vector<std::string> result;
            result.reserve(vector.size());
            for (const auto& item : vector) {
                if (item.is_string()) {
                    result.emplace_back(item.get_string());
                }
            }
            return result;
        }
        return empty_vector;
    }

    [[nodiscard]] auto get_string(std::string_view key) const -> const std::string&
    {
        static const std::string empty_string;
        if (const auto* value = ctx_.find(key); value != nullptr && value->is_string()) {
            return value->get_string();
        }
        return empty_string;
    }

    template<typename Number>
    [[nodiscard]] auto get_number(std::string_view key) const -> Number
    {
        if (const auto* value = ctx_.find(key); value != nullptr && value->is_number()) {
            return value->as<Number>();
        }
        return {};
    }

  private:
    core_operation_info info_;
    tao::json::value ctx_;
};

core_error_context::core_error_context(core_error_context&& other, tao::json::value&& extra)
  : impl_{ std::move(other.impl_) }
{
    impl_->merge(std::move(extra));
}

core_error_context::core_error_context(core_operation_info info, tao::json::value&& ctx)
  : impl_{ std::make_unique<core_error_context_impl>(std::move(info), std::move(ctx)) }
{
}

core_error_context::core_error_context(core_error_context&& other) noexcept
  : impl_{ std::move(other.impl_) }
{
}

core_error_context&
core_error_context::operator=(core_error_context&& other) noexcept
{
    impl_ = std::move(other.impl_);
    return *this;
}

auto
core_error_context::to_json_string() const -> std::string
{
    Expects(impl_);
    return impl_->to_json_string();
}

auto
core_error_context::ec() const -> std::error_code
{
    Expects(impl_);
    return impl_->info().ec_;
}

auto
core_error_context::last_dispatched_to() const -> const std::optional<std::string>&
{
    Expects(impl_);
    return impl_->info().last_dispatched_to_;
}

auto
core_error_context::last_dispatched_from() const -> const std::optional<std::string>&
{
    Expects(impl_);
    return impl_->info().last_dispatched_from_;
}

auto
core_error_context::retry_attempts() const -> std::size_t
{
    Expects(impl_);
    return impl_->info().retry_attempts_;
}

auto
core_error_context::retry_reasons() const -> const std::set<retry_reason>&
{
    Expects(impl_);
    return impl_->info().retry_reasons_;
}

class core_view_error_context_impl
{
  public:
    std::optional<std::vector<std::string>> query_string_{};
};

core_view_error_context::core_view_error_context(core_error_context&& ctx)
  : core_error_context(std::move(ctx))
  , impl_{ std::make_unique<core_view_error_context_impl>() }
{
}

core_view_error_context::core_view_error_context(core_error_context&& ctx, tao::json::value&& extra)
  : core_error_context(std::move(ctx), std::move(extra))
  , impl_{ std::make_unique<core_view_error_context_impl>() }
{
}

auto
core_view_error_context::design_document_name() const -> const std::string&
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_string("design_document_name");
}

auto
core_view_error_context::view_name() const -> const std::string&
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_string("view_name");
}

auto
core_view_error_context::http_status() const -> std::uint32_t
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_number<std::uint32_t>("http_status");
}

auto
core_view_error_context::http_body() const -> const std::string&
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_string("http_body");
}

auto
core_view_error_context::client_context_id() const -> const std::string&
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_string("client_context_id");
}

auto
core_view_error_context::method() const -> const std::string&
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_string("method");
}

auto
core_view_error_context::path() const -> const std::string&
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_string("path");
}

auto
core_view_error_context::hostname() const -> const std::string&
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_string("hostname");
}

auto
core_view_error_context::port() const -> std::uint16_t
{
    Expects(core_error_context::impl_);
    return core_error_context::impl_->get_number<std::uint16_t>("port");
}

auto
core_view_error_context::query_string() const -> const std::vector<std::string>&
{
    Expects(core_error_context::impl_);
    if (!impl_->query_string_) {
        impl_->query_string_ = core_error_context::impl_->get_vector_of_string("query_string");
    }

    return impl_->query_string_.value();
}
} // namespace couchbase
