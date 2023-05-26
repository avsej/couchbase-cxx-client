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
#include "core/query_context.hxx"

#include <couchbase/watch_query_indexes_options.hxx>

#include <memory>
#include <string>
#include <vector>

namespace couchbase::core::impl
{
void
initiate_watch_query_indexes(couchbase::core::cluster core,
                             std::string bucket_name,
                             std::vector<std::string> index_names,
                             couchbase::watch_query_indexes_options::built options,
                             query_context query_ctx,
                             std::string collection_name,
                             watch_query_indexes_handler&& handler);

void
initiate_watch_query_indexes(couchbase::core::cluster core,
                             std::string bucket_name,
                             std::vector<std::string> index_names,
                             couchbase::watch_query_indexes_options::built options,
                             watch_query_indexes_handler&& handler);
} // namespace couchbase::core::impl
