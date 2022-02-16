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

#include "error_map_component.hxx"
#include "error_map.hxx"

#include "../mcbp/datatype.hxx"
#include "../mcbp_queue_request.hxx"
#include "../mcbp_queue_response.hxx"

#include "core/logger/logger.hxx"
#include "core/utils/json.hxx"

#include <algorithm>

namespace couchbase::new_core::impl
{
error_map_component::error_map_component(std::string bucket_name)
  : bucket_name_{ std::move(bucket_name) }
{
}

auto
error_map_component::get_error_map_data(mcbp::status_code code) const -> const error_map_error*
{
    if (error_map_ == nullptr) {
        return nullptr;
    }

    if (auto it = error_map_->errors.find(static_cast<std::uint16_t>(code)); it != error_map_->errors.end()) {
        return &it->second;
    }

    return nullptr;
}

void
error_map_component::store_error_map(const std::vector<std::byte>& error_map_bytes)
{
    if (auto [new_error_map, err] = parse_key_value_error_map(error_map_bytes); err) {
        LOG_DEBUG("failed to parse key/value error map: {}", err.message());
    } else {
        std::scoped_lock lock(error_map_mutex_);
        if (error_map_ == nullptr || error_map_->revision < new_error_map->revision) {
            std::swap(error_map_, new_error_map);
        }
    }
}

auto
error_map_component::should_retry(mcbp::status_code code) const -> bool
{
    const auto* error_data = get_error_map_data(code);

    if (error_data == nullptr) {
        return false;
    }

    return std::any_of(error_data->attributes.begin(), error_data->attributes.end(), [](const auto& attribute) {
        return attribute == "auto-retry" || attribute == "retry-now" || attribute == "retry-later";
    });
}

auto
error_map_component::enhance_kv_error(std::error_code error,
                                      std::shared_ptr<mcbp_queue_response> response,
                                      std::shared_ptr<mcbp_queue_request> request) const -> key_value_error
{
    key_value_error enhanced_error{ error };

    enhanced_error.bucket_name = bucket_name_;

    if (request != nullptr) {
        enhanced_error.document_key.assign(reinterpret_cast<const char*>(request->key_.data()), request->key_.size());
        enhanced_error.collection_name = request->collection_name_;
        enhanced_error.scope_name = request->scope_name_;
        enhanced_error.collection_id = request->collection_id_;
        auto [count, reasons] = request->retries();
        enhanced_error.retry_attempts = count;
        enhanced_error.retry_reasons = std::move(reasons);
        auto connection_info = request->connection_info();
        enhanced_error.last_dispatched_to = connection_info.last_dispatched_to;
        enhanced_error.last_dispatched_from = connection_info.last_dispatched_from;
        enhanced_error.last_connection_id = connection_info.last_connection_id;
    }

    if (response != nullptr) {
        enhanced_error.status = response->status_;
        enhanced_error.opaque = response->opaque_;
        if (const auto* data = get_error_map_data(enhanced_error.status); data != nullptr) {
            enhanced_error.error_name = data->name;
            enhanced_error.error_description = data->description;
        }
        if (mcbp::has_json_datatype(response->datatype_) && !response->value_.empty()) {
            if (auto json = core::utils::json::parse_binary(response->value_); json.is_object()) {
                if (const auto* err_obj = json.find("error"); err_obj != nullptr && err_obj->is_object()) {
                    if (const auto* ref = err_obj->find("ref"); ref != nullptr && ref->is_string()) {
                        enhanced_error.error_reference = ref->get_string();
                    }
                    if (const auto* ctx = err_obj->find("context"); ctx != nullptr && ctx->is_string()) {
                        enhanced_error.error_context = ctx->get_string();
                    }
                }
            }
        }
    }

    return enhanced_error;
}
} // namespace couchbase::new_core::impl
