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

#pragma once

#include <set>

namespace couchbase::new_core
{
enum class bucket_capability {
    cbhello,
    cccp,
    collections,
    couchapi,
    dcp,
    durable_write,
    nodes_ext,
    preserve_expiry,
    range_scan,
    subdoc_document_macro_support,
    subdoc_replace_body_with_xattr,
    subdoc_revive_document,
    tombstoned_user_xattrs,
    touch,
    xattr,
    xdcr_checkpointing,
};

class bucket_capabilities
{
  public:
    bucket_capabilities() = default;
    bucket_capabilities(const bucket_capabilities&) = default;
    bucket_capabilities(bucket_capabilities&&) = default;
    auto operator=(const bucket_capabilities&) -> bucket_capabilities& = default;
    auto operator=(bucket_capabilities&&) -> bucket_capabilities& = default;

    explicit bucket_capabilities(std::set<bucket_capability> capabilities)
      : capabilities_{ std::move(capabilities) }
    {
    }

    [[nodiscard]] auto supports(bucket_capability capability) const -> bool
    {
        return capabilities_.find(capability) != capabilities_.end();
    }

  private:
    std::set<bucket_capability> capabilities_{};
};

} // namespace couchbase::new_core
