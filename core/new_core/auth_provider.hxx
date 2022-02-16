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

#include "auth_certificate_request.hxx"
#include "auth_credentials.hxx"
#include "auth_credentials_request.hxx"
#include "auth_mechanism.hxx"

#include <string>
#include <system_error>
#include <vector>

namespace couchbase::new_core
{
class auth_provider
{
  public:
    virtual ~auth_provider() = default;

    [[nodiscard]] virtual auto supports_tls() const -> bool = 0;
    [[nodiscard]] virtual auto supports_non_tls() const -> bool = 0;
    [[nodiscard]] virtual auto certificate(const auth_certificate_request& request) const -> std::pair<std::string, std::error_code> = 0;
    [[nodiscard]] virtual auto credentials(const auth_credentials_request& request) const
      -> std::pair<auth_credentials, std::error_code> = 0;
    [[nodiscard]] virtual auto allowed_mechanisms() const -> std::vector<auth_mechanism> = 0;
};
} // namespace couchbase::new_core
