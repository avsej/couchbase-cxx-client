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

#include "impl/config_management_component.hxx"

#include "config_management_component.hxx"

#include <memory>

namespace couchbase::new_core
{

config_management_component::config_management_component(const config_manager_properties& properties)
  : impl_{ std::make_shared<impl::config_management_component>(properties) }
{
}

void
config_management_component::use_tls(bool use)
{
    impl_->use_tls(use);
}

const std::string&
config_management_component::network_type() const
{
    return impl_->network_type();
}

void
config_management_component::add_config_watcher(std::shared_ptr<route_config_watcher> watcher)
{
    impl_->add_config_watcher(std::move(watcher));
}

void
config_management_component::remove_config_watcher(std::shared_ptr<route_config_watcher> watcher)
{
    impl_->remove_config_watcher(std::move(watcher));
}

void
config_management_component::on_new_config(const config_value& config) const
{
    impl_->on_new_config(config);
}
} // namespace couchbase::new_core
