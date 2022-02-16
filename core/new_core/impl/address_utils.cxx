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

#include "address_utils.hxx"

namespace couchbase::new_core::utils
{
std::string
host_from_host_port(const std::string& hostport)
{
    auto last_colon = hostport.find(':');
    auto last_bracket = hostport.find(']');
    if (last_bracket == std::string::npos) {
        // not an IPv6 address
        if (last_colon == std::string::npos) {
            return hostport;
        }
        return hostport.substr(0, last_colon);
    }
    if (last_colon > last_bracket) {
        // IPv6 with port
        return hostport.substr(0, last_colon);
    }
    return hostport;
}
} // namespace couchbase::new_core::utils
