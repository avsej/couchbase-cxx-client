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

#pragma once

#include "pipeline_snapshot.hxx"

#include <system_error>
#include <utility>

#include <asio/io_context.hpp>

namespace couchbase::new_core
{
class mcbp_dispatcher
{
  public:
    explicit mcbp_dispatcher(std::shared_ptr<asio::io_context> io);

    [[nodiscard]] auto get_pipeline_snapshot() const -> std::pair<pipeline_snapshot, std::error_code>;
#if 0
    DispatchDirect(req *memdQRequest) (PendingOp, error)
	RequeueDirect(req *memdQRequest, isRetry bool)
	DispatchDirectToAddress(req *memdQRequest, pipeline *memdPipeline) (PendingOp, error)
	CollectionsEnabled() bool
	SupportsCollections() bool
	SetPostCompleteErrorHandler(handler postCompleteErrorHandler)
#endif

  private:
    std::shared_ptr<asio::io_context> io_{};
};
} // namespace couchbase::new_core
