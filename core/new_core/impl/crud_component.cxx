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

#include "crud_component.hxx"
#include "big_endian.hxx"
#include "tracer_component.hxx"
#include "tracer_constants.hxx"

#include "../mcbp_queue_request.hxx"
#include "../mcbp_queue_response.hxx"
#include "../ops.hxx"

#include <couchbase/error_codes.hxx>

#include <memory>

namespace couchbase::new_core::impl
{
auto
crud_component::get(ops::get_operation operation_) -> completion_token
{
    auto operation = std::move(operation_);
    auto tracer = tracer_component_->start_telemetry_handler(metric_value_service_key_value, "Get");

    auto handler = [tracer = std::move(tracer), callback = std::move(operation.callback)](std::shared_ptr<mcbp_queue_response> response,
                                                                                          std::shared_ptr<mcbp_queue_request> /* request */,
                                                                                          std::error_code err) {
        ops::get_result result{};

        if (err) {
            tracer->finish();
            return callback(std::move(result), err);
        }

        if (response->extras_.size() == 4) {
            tracer->finish();
            return callback(std::move(result), errc::network::protocol_error);
        }

        result.value = std::move(response->value_);
        result.cas = response->cas_;
        result.datatype = response->datatype_;
        result.flags = big_endian::read_uint32(response->extras_, 0);
        if (response->read_units_frame_ && response->write_units_frame_) {
            ops::resource_unit_result units{};
            if (response->read_units_frame_) {
                units.read_units = response->read_units_frame_->read_units;
            }
            if (response->write_units_frame_) {
                units.write_units = response->write_units_frame_->write_units;
            }
            result.internal.resource_units = units;
        }
        tracer->finish();

        callback(result, {});
    };

    return completion_token();
}
} // namespace couchbase::new_core::impl
