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

#include "endpoint_state.hxx"

namespace couchbase::new_core
{
std::string
to_string(endpoint_state state)
{
    switch (state) {
        case endpoint_state::disconnected:
            return "disconnected";
        case endpoint_state::connecting:
            return "connecting";
        case endpoint_state::connected:
            return "connected";
        case endpoint_state::disconnecting:
            return "disconnecting";
    }
    return "invalid";
}
} // namespace couchbase::new_core
