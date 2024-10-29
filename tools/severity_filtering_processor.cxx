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

#include "severity_filtering_processor.hxx"

#include <opentelemetry/sdk/logs/readable_log_record.h>

namespace couchbase::observability
{
severity_filtering_processor::severity_filtering_processor(
  std::unique_ptr<opentelemetry::sdk::logs::LogRecordProcessor> processor,
  opentelemetry::logs::Severity severity_threshold)
  : processor_{ std::move(processor) }
  , severity_threshold_{ severity_threshold }
{
}
void
severity_filtering_processor::OnEmit(
  std::unique_ptr<opentelemetry::sdk::logs::Recordable>&& record) noexcept
{
  const auto* log_record =
    dynamic_cast<const opentelemetry::sdk::logs::ReadableLogRecord*>(record.get());
  if (log_record != nullptr) {
    if (log_record->GetSeverity() >= severity_threshold_) {
      {
        processor_->OnEmit(std::move(record));
      }
    }
  }
}

auto
severity_filtering_processor::ForceFlush(std::chrono::microseconds timeout) noexcept -> bool
{
  return processor_->ForceFlush(timeout);
}

auto
severity_filtering_processor::Shutdown(std::chrono::microseconds timeout) noexcept -> bool
{
  return processor_->Shutdown(timeout);
}

auto
severity_filtering_processor::MakeRecordable() noexcept
  -> std::unique_ptr<opentelemetry::sdk::logs::Recordable>
{
  return processor_->MakeRecordable();
}
} // namespace couchbase::observability
