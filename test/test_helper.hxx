/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2020-2021 Couchbase, Inc.
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

#include "utils/binary.hxx"
#include "utils/test_context.hxx"
#include "utils/test_data.hxx"

#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_VOID_CAST_EXPRESSIONS
#include <doctest/doctest.h>

#define SAFE_INFO(ec_name, ec)                                                                                                             \
    auto ec_name = ec;                                                                                                                     \
    INFO(ec_name);

#define REQUIRE_SUCCESS(ec)                                                                                                                \
    SAFE_INFO(DOCTEST_ANONYMOUS(ERROR_CODE_), (ec).message());                                                                             \
    REQUIRE_FALSE(ec)

#define EXPECT_SUCCESS(result)                                                                                                             \
    if (!result) {                                                                                                                         \
        SAFE_INFO(DOCTEST_ANONYMOUS(ERROR_CODE_), result.error().message());                                                               \
    }                                                                                                                                      \
    REQUIRE(result)
