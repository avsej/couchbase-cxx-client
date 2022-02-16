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

#include "ops/analytics_operation.hxx"
#include "ops/append_operation.hxx"
#include "ops/decrement_operation.hxx"
#include "ops/freeform_http_operation.hxx"
#include "ops/get_all_collection_manifests_operation.hxx"
#include "ops/get_and_lock_operation.hxx"
#include "ops/get_and_touch_operation.hxx"
#include "ops/get_collection_id_operation.hxx"
#include "ops/get_collection_manifest_operation.hxx"
#include "ops/get_meta_operation.hxx"
#include "ops/get_one_replica_operation.hxx"
#include "ops/get_operation.hxx"
#include "ops/get_random_operation.hxx"
#include "ops/increment_operation.hxx"
#include "ops/insert_operation.hxx"
#include "ops/lookup_in_operation.hxx"
#include "ops/mutate_in_operation.hxx"
#include "ops/observe_operation.hxx"
#include "ops/observe_vbucket_operation.hxx"
#include "ops/ping_operation.hxx"
#include "ops/prepared_query_operation.hxx"
#include "ops/prepend_operation.hxx"
#include "ops/query_operation.hxx"
#include "ops/remove_meta_operation.hxx"
#include "ops/remove_operation.hxx"
#include "ops/replace_operation.hxx"
#include "ops/search_operation.hxx"
#include "ops/set_meta_operation.hxx"
#include "ops/stats_operation.hxx"
#include "ops/touch_operation.hxx"
#include "ops/unlock_operation.hxx"
#include "ops/upsert_operation.hxx"
#include "ops/view_operation.hxx"

#include "completion_token.hxx"
