/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2025-Present Couchbase, Inc.
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

#include <opentelemetry/logs/log_record.h>
#include <opentelemetry/logs/severity.h>

#include <opentelemetry/sdk/logs/processor.h>
#include <opentelemetry/sdk/logs/recordable.h>

#include <memory>

namespace couchbase::observability
{
class severity_filtering_processor : public opentelemetry::sdk::logs::LogRecordProcessor
{
public:
  explicit severity_filtering_processor(
    std::unique_ptr<opentelemetry::sdk::logs::LogRecordProcessor> processor,
    opentelemetry::logs::Severity severity_threshold);

  auto MakeRecordable() noexcept -> std::unique_ptr<opentelemetry::sdk::logs::Recordable> override;

  void OnEmit(std::unique_ptr<opentelemetry::sdk::logs::Recordable>&& record) noexcept override;

  auto ForceFlush(std::chrono::microseconds timeout) noexcept -> bool override;

  auto Shutdown(std::chrono::microseconds timeout) noexcept -> bool override;

private:
  std::unique_ptr<opentelemetry::sdk::logs::LogRecordProcessor> processor_;
  opentelemetry::logs::Severity severity_threshold_;
};
} // namespace couchbase::observability
