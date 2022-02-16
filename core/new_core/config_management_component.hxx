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

#include "config_manager.hxx"
#include "config_manager_properties.hxx"
#include "config_value.hxx"
#include "impl/fwd/config_management_component.hxx"

#include <memory>
#include <string>
#include <vector>

namespace couchbase::new_core
{
class config_management_component : public config_manager
{
  public:
    explicit config_management_component(const config_manager_properties& properties);
    config_management_component(const config_management_component& other) = default;
    config_management_component(config_management_component&& other) = default;
    auto operator=(const config_management_component& other) -> config_management_component& = default;
    auto operator=(config_management_component&& other) -> config_management_component& = default;

    void use_tls(bool use);
    void add_config_watcher(std::shared_ptr<route_config_watcher> watcher) override;
    void remove_config_watcher(std::shared_ptr<route_config_watcher> watcher) override;
    [[nodiscard]] const std::string& network_type() const;
    void on_new_config(const config_value& config) const;

  private:
    std::shared_ptr<impl::config_management_component> impl_;
};
} // namespace couchbase::new_core
