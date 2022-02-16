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

namespace couchbase::new_core
{
namespace ops
{
struct analytics_operation;
struct append_operation;
struct decrement_operation;
struct freeform_http_operation;
struct get_all_collection_manifests_operation;
struct get_and_lock_operation;
struct get_and_touch_operation;
struct get_collection_id_operation;
struct get_collection_manifest_operation;
struct get_meta_operation;
struct get_one_replica_operation;
struct get_result;
struct get_operation;
struct get_random_operation;
struct increment_operation;
struct insert_operation;
struct lookup_in_operation;
struct mutate_in_operation;
struct observe_operation;
struct observe_vbucket_operation;
struct ping_operation;
struct prepared_query_operation;
struct prepend_operation;
struct query_operation;
struct remove_meta_operation;
struct remove_operation;
struct replace_operation;
struct search_operation;
struct set_meta_operation;
struct stats_operation;
struct touch_operation;
struct unlock_operation;
struct upsert_operation;
struct view_operation;
} // namespace ops
struct completion_token;
} // namespace couchbase::new_core
