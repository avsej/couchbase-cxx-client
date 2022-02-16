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

#include "certificate_auth_provider.hxx"

namespace couchbase::new_core
{
certificate_auth_provider::certificate_auth_provider(std::string client_certificate)
  : client_certificate_{ std::move(client_certificate) }
{
}

auto
certificate_auth_provider::supports_tls() const -> bool
{
    return true;
}

auto
certificate_auth_provider::supports_non_tls() const -> bool
{
    return false;
}

auto
certificate_auth_provider::certificate(const auth_certificate_request& /* request */) const -> std::pair<std::string, std::error_code>
{
    return { client_certificate_, {} };
}

auto
certificate_auth_provider::credentials(const auth_credentials_request& /* request */) const -> std::pair<auth_credentials, std::error_code>
{
    return { {}, {} };
}

auto
certificate_auth_provider::allowed_mechanisms() const -> std::vector<auth_mechanism>
{
    return {};
}
} // namespace couchbase::new_core
