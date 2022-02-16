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

#include "auth_provider.hxx"

namespace couchbase::new_core
{
class password_auth_provider : public auth_provider
{
  public:
    password_auth_provider(std::string username, std::string password);
    [[nodiscard]] static auto ldap_compatible(std::string username, std::string password) -> password_auth_provider;

    [[nodiscard]] auto supports_tls() const -> bool override;
    [[nodiscard]] auto supports_non_tls() const -> bool override;
    [[nodiscard]] auto certificate(const auth_certificate_request& request) const -> std::pair<std::string, std::error_code> override;
    [[nodiscard]] auto credentials(const auth_credentials_request& request) const -> std::pair<auth_credentials, std::error_code> override;
    [[nodiscard]] auto allowed_mechanisms() const -> std::vector<auth_mechanism> override;

  private:
    std::string username_;
    std::string password_;
    std::vector<auth_mechanism> allowed_mechanisms_{
        auth_mechanism::scram_sha512,
        auth_mechanism::scram_sha256,
        auth_mechanism::scram_sha1,
    };
};
} // namespace couchbase::new_core
