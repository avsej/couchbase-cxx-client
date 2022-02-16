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

#include "impl/error_map_component.hxx"

namespace couchbase::new_core
{
error_map_component::error_map_component(std::string bucket_name)
  : impl_{ std::make_shared<impl::error_map_component>(std::move(bucket_name)) }
{
}

auto
error_map_component::should_retry(mcbp::status_code code) const -> bool
{
    return impl_->should_retry(code);
}

auto
error_map_component::enhance_kv_error(std::error_code error,
                                      std::shared_ptr<mcbp_queue_response> response,
                                      std::shared_ptr<mcbp_queue_request> request) const -> key_value_error
{
    return impl_->enhance_kv_error(error, response, request);
}

void
error_map_component::store_error_map(const std::vector<std::byte>& error_map_bytes)
{
    impl_->store_error_map(error_map_bytes);
}
} // namespace couchbase::new_core
