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

#include "auth_mechanism.hxx"

#include <couchbase/error_codes.hxx>

namespace couchbase::new_core
{
std::string
to_string(auth_mechanism mechanism)
{
    switch (mechanism) {
        case auth_mechanism::plain:
            return "PLAIN";
        case auth_mechanism::scram_sha1:
            return "SCRAM-SHA1";
        case auth_mechanism::scram_sha256:
            return "SCRAM-SHA256";
        case auth_mechanism::scram_sha512:
            return "SCRAM-SHA512";
    }
    throw std::system_error(errc::common::invalid_argument, "unknown SASL mechanism: " + std::to_string(static_cast<int>(mechanism)));
}
} // namespace couchbase::new_core
