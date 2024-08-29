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

#include <atomic>
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

#define CATCH_CONFIG_MAIN

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include "include_ssl/ssl.h"

class TracyTestListener : public Catch::EventListenerBase
{
private:
  TracyCZoneCtx currentZone;

public:
  using Catch::EventListenerBase::EventListenerBase;

  void testCaseStarting(const Catch::TestCaseInfo& testInfo) override
  {
    const auto& lineInfo = testInfo.lineInfo;

    uint64_t srcloc = ___tracy_alloc_srcloc_name(
      /* line = */ lineInfo.line,
      /* source = */ lineInfo.file,
      /* sourceLen = */ strlen(lineInfo.file),
      /* function = */ "",
      /* functionLen = */ 0,
      /* name = */ testInfo.name.c_str(),
      /* nameLen = */ testInfo.name.size(),
      /* color = */ 0);

    currentZone = ___tracy_emit_zone_begin_alloc(
      /* srcloc = */ srcloc,
      /* active = */ 1);
  }

  void testCaseEnded(const Catch::TestCaseStats& /* testCaseStats */) override
  {
    TracyCZoneEnd(currentZone);
  }
};

CATCH_REGISTER_LISTENER(TracyTestListener)

auto
main(int argc, char* argv[]) -> int
{
  int result = Catch::Session().run(argc, argv);

  OPENSSL_cleanup();

  return result;
}
