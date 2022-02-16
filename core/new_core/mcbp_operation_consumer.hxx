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

#include <memory>

namespace couchbase::new_core
{
class mcbp_operation_queue;
class mcbp_queue_request;

class mcbp_operation_consumer : public std::enable_shared_from_this<mcbp_operation_consumer>
{
  public:
    explicit mcbp_operation_consumer(std::shared_ptr<mcbp_operation_queue> parent);

    [[nodiscard]] auto queue() -> std::shared_ptr<mcbp_operation_queue>;
    [[nodiscard]] auto pop() -> std::shared_ptr<mcbp_queue_request>;
    void close();

  private:
    std::shared_ptr<mcbp_operation_queue> parent_;
    bool is_closed_{ false };

    friend mcbp_operation_queue;
};
} // namespace couchbase::new_core
