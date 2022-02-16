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

#include "mcbp_connection.hxx"
#include "big_endian.hxx"
#include "buffer_writer.hxx"

#include "core/logger/logger.hxx"
#include "core/new_core/errors.hxx"
#include "core/new_core/mcbp/server_duration.hxx"
#include "core/utils/unsigned_leb128.hxx"

#include <couchbase/error_codes.hxx>

namespace couchbase::new_core::impl
{

const std::string&
mcbp_connection::local_address() const
{
    return local_address_;
}

const std::string&
mcbp_connection::remote_address() const
{
    return remote_address_;
}

std::error_code
mcbp_connection::write_packet(const mcbp::packet& packet)
{
    auto encoded_key = packet.key_;
    auto extras = packet.extras_;

    if (collections_enabled_) {
        if (packet.command_ == mcbp::command_code::observe) {
            // While it's possible that the Observe operation is in fact supported with collections
            // enabled, we don't currently implement that operation for simplicity, as the key is
            // actually hidden away in the value data instead of the usual key data.
            LOG_DEBUG("the observe operation is not supported with collections enabled");
            return errc::common::unsupported_operation;
        }
        if (supports_collection_id(packet.command_)) {
            core::utils::unsigned_leb128<std::uint32_t> encoded(packet.collection_id_);
            encoded_key.reserve(encoded_key.size() + encoded.size());
            encoded_key.insert(encoded_key.begin(), encoded.begin(), encoded.end());
        } else if (packet.command_ == mcbp::command_code::get_random) {
            // GetRandom expects the cid to be in the extras
            // GetRandom MUST not have any extras if not using collections, so we're ok to just set it.
            // It also doesn't expect the collection ID to be leb encoded.
            extras.resize(sizeof(std::uint32_t));
            big_endian::put_uint32(extras, packet.collection_id_);
        }
        if (packet.collection_id_ > 0) {
            LOG_DEBUG("cannot encode collection id with a non-collection command");
            return errc::common::invalid_argument;
        }
    }

    std::size_t ext_len = extras.size();
    std::size_t key_len = encoded_key.size();
    std::size_t val_len = packet.value_.size();
    std::size_t frames_len = 0;

    if (packet.barrier_frame_) {
        frames_len += 1;
    }
    if (packet.durability_level_frame_) {
        frames_len += 2;
        if (packet.durability_timeout_frame_) {
            frames_len += 2;
        }
    }
    if (packet.stream_id_frame_) {
        frames_len += 3;
    }
    if (packet.open_tracing_frame_) {
        std::size_t trace_ctx_len = packet.open_tracing_frame_->trace_context.size();
        frames_len = trace_ctx_len;
        if (trace_ctx_len < 15) {
            frames_len += 1;
        } else {
            frames_len += 2;
        }
    }
    if (packet.server_duration_frame_) {
        frames_len += 3;
    }
    if (packet.user_impersonation_frame_) {
        std::size_t user_len = packet.user_impersonation_frame_->user.size();
        frames_len += user_len;
        if (user_len < 15) {
            frames_len += 1;
        } else {
            frames_len += 2;
        }
    }
    if (packet.preserve_expiry_frame_) {
        frames_len += 1;
    }

    // We automatically upgrade a packet from normal Req or Res magic into the frame variant depending on the usage of them.
    auto packet_magic = packet.magic_;
    if (frames_len > 0) {
        switch (packet_magic) {
            case mcbp::command_magic::request:
                if (!is_feature_enabled(mcbp::hello_feature::alt_requests)) {
                    LOG_DEBUG("cannot use frames in req packets without enabling the feature");
                    return errc::common::unsupported_operation;
                }
                packet_magic = mcbp::command_magic::request_ext;
                break;
            case mcbp::command_magic::response:
                packet_magic = mcbp::command_magic::response_ext;
                break;
            default:
                LOG_DEBUG("cannot use frames with an unsupported magic");
                return errc::common::unsupported_operation;
        }
    }
    std::size_t packet_len = 24 + ext_len + frames_len + key_len + val_len;
    buffer_writer buffer{ packet_len, &memory_pool_ };
    buffer.write_byte(std::byte{ packet_magic });
    buffer.write_byte(std::byte{ packet.command_ });

    // This is safe to do without checking the magic as we check the magic above before incrementing the framesLen variable
    if (frames_len > 0) {
        buffer.write_byte(static_cast<std::byte>(frames_len));
        buffer.write_byte(static_cast<std::byte>(key_len));
    } else {
        buffer.write_uint16(static_cast<std::uint16_t>(key_len));
    }
    buffer.write_byte(static_cast<std::byte>(ext_len));
    buffer.write_byte(packet.datatype_);

    switch (packet.magic_) {
        case mcbp::command_magic::request:
        case mcbp::command_magic::request_ext:
            if (static_cast<std::uint32_t>(packet.status_) != 0) {
                LOG_DEBUG("cannot specify status in a request packet");
                return errc::common::invalid_argument;
            }
            buffer.write_uint16(packet.vbucket_);
            break;

        case mcbp::command_magic::response:
        case mcbp::command_magic::response_ext:
            if (static_cast<std::uint32_t>(packet.vbucket_) != 0) {
                LOG_DEBUG("cannot specify vbucket in a response packet");
                return errc::common::invalid_argument;
            }
            buffer.write_uint16(static_cast<std::uint16_t>(packet.status_));
            break;

        default:
            LOG_DEBUG("cannot encode status/vbucket for unknown packet magic");
            return errc::common::invalid_argument;
    }

    buffer.write_uint32(static_cast<std::uint32_t>(key_len + ext_len + val_len + frames_len));
    buffer.write_uint32(packet.opaque_);
    buffer.write_uint64(packet.cas_);

    // Generate the framing extra data

    if (packet.barrier_frame_) {
        if (packet.magic_ != mcbp::command_magic::request) {
            LOG_DEBUG("cannot use barrier frame in non-request packets");
            return errc::common::invalid_argument;
        }
        buffer.write_frame_header(mcbp::request_barrier, 0);
    }

    if (packet.durability_level_frame_) {
        if (packet.magic_ != mcbp::command_magic::request) {
            LOG_DEBUG("cannot use durability level frame in non-request packets");
            return errc::common::invalid_argument;
        }

        if (!is_feature_enabled(mcbp::hello_feature::sync_replication)) {
            LOG_DEBUG("cannot use sync replication frames without enabling the feature");
            return errc::common::feature_not_available;
        }

        if (packet.durability_timeout_frame_) {
            auto millis = packet.durability_timeout_frame_->timeout.count();
            if (millis > 65535) {
                millis = 65535;
            }
            buffer.write_frame_header(mcbp::request_sync_durability, 3);
            buffer.write_byte(static_cast<std::byte>(packet.durability_level_frame_->level));
            buffer.write_uint16(static_cast<std::uint16_t>(millis));
        } else {
            buffer.write_frame_header(mcbp::request_sync_durability, 1);
            buffer.write_byte(static_cast<std::byte>(packet.durability_level_frame_->level));
        }
    }

    if (packet.stream_id_frame_) {
        if (packet.magic_ != mcbp::command_magic::request) {
            LOG_DEBUG("cannot use stream id frame in non-request packets");
            return errc::common::invalid_argument;
        }

        buffer.write_frame_header(mcbp::request_stream_id, 2);
        buffer.write_uint16(packet.stream_id_frame_->stream_id);
    }

    if (packet.open_tracing_frame_) {
        if (packet.magic_ != mcbp::command_magic::request) {
            LOG_DEBUG("cannot use open tracing frame in non-request packets");
            return errc::common::invalid_argument;
        }

        if (!is_feature_enabled(mcbp::hello_feature::open_tracing)) {
            LOG_DEBUG("cannot use open tracing frames without enabling the feature");
            return errc::common::feature_not_available;
        }

        std::size_t trace_ctx_len = packet.open_tracing_frame_->trace_context.size();
        if (trace_ctx_len < 15) {
            buffer.write_frame_header(mcbp::request_open_tracing, trace_ctx_len);
            buffer.write(packet.open_tracing_frame_->trace_context);
        } else {
            buffer.write_frame_header(mcbp::request_open_tracing, 15);
            buffer.write_byte(static_cast<std::byte>(trace_ctx_len - 15));
            buffer.write(packet.open_tracing_frame_->trace_context);
        }
    }

    if (packet.server_duration_frame_) {
        if (packet.magic_ != mcbp::command_magic::response) {
            LOG_DEBUG("cannot use server duration frame in non-response packets");
            return errc::common::invalid_argument;
        }

        if (!is_feature_enabled(mcbp::hello_feature::durations)) {
            buffer.write_frame_header(mcbp::response_server_duration, 2);
            buffer.write_uint16(mcbp::encode_server_duration(packet.server_duration_frame_->server_duration));
        }
    }

    if (packet.user_impersonation_frame_) {
        if (packet.magic_ != mcbp::command_magic::request) {
            LOG_DEBUG("cannot use user impersonation frame in non-request packets");
            return errc::common::invalid_argument;
        }

        std::size_t user_len = packet.user_impersonation_frame_->user.size();
        if (user_len < 15) {
            buffer.write_frame_header(mcbp::request_user_impersonation, user_len);
            buffer.write(packet.user_impersonation_frame_->user);
        } else {
            buffer.write_frame_header(mcbp::request_user_impersonation, 15);
            buffer.write_byte(static_cast<std::byte>(user_len - 15));
            buffer.write(packet.user_impersonation_frame_->user);
        }
    }

    if (packet.preserve_expiry_frame_) {
        if (packet.magic_ != mcbp::command_magic::request) {
            LOG_DEBUG("cannot use preserve expiry frame in non-request packets");
            return errc::common::invalid_argument;
        }

        if (!is_feature_enabled(mcbp::hello_feature::preserve_expiry)) {
            LOG_DEBUG("cannot use preserve expiry frame without enabling the feature");
            return errc::common::feature_not_available;
        }

        buffer.write_frame_header(mcbp::request_preserve_expiry, 0);
    }

    if (!packet.unsupported_frames_.empty()) {
        LOG_DEBUG("cannot use send packets with unsupported frames");
        return errc::common::invalid_argument;
    }

    // Copy the extras into the body of the packet
    buffer.write(extras);

    // Copy the encoded key into the body of the packet
    buffer.write(encoded_key);

    // Copy the value into the body of the packet
    buffer.write(packet.value_);

    auto [bytes_written, err] = stream_->write(buffer.store.data(), buffer.store.size());
    if (err) {
        return err;
    }
    if (bytes_written != buffer.store.size()) {
        return core::core_errc::short_write;
    }

    return {};
}

std::tuple<mcbp::packet, std::size_t, std::error_code>
mcbp_connection::read_packet()
{
    mcbp::packet packet;

    if (stream_ == nullptr) {
        return { {}, {}, core::core_errc::end_of_file };
    }

    // Read the entire 24-byte header first
    if (auto [_, err] = stream_->read(header_buffer_.data(), header_buffer_.size()); err) {
        return { {}, {}, err };
    }

    // grab the length of the full body
    std::uint32_t body_len = big_endian::read_uint32(header_buffer_, 8);
    std::vector<std::byte> body;
    body.resize(body_len);

    // Read the remaining bytes of the body
    if (auto [_, err] = stream_->read(body.data(), body.size()); err) {
        return { {}, {}, err };
    }

    auto magic = static_cast<mcbp::command_magic>(header_buffer_[0]);

    switch (magic) {
        case mcbp::command_magic::request:
        case mcbp::command_magic::request_ext:
            packet.magic_ = mcbp::command_magic::request;
            packet.vbucket_ = big_endian::read_uint16(header_buffer_, 6);
            break;

        case mcbp::command_magic::response:
        case mcbp::command_magic::response_ext:
            packet.magic_ = mcbp::command_magic::response;
            packet.status_ = static_cast<mcbp::status_code>(big_endian::read_uint16(header_buffer_, 6));
            break;

        default:
            LOG_DEBUG("cannot decode status/vbucket for unknown packet magic");
            return { {}, {}, errc::network::protocol_error };
    }

    packet.command_ = static_cast<mcbp::command_code>(header_buffer_[1]);
    packet.datatype_ = header_buffer_[5];
    packet.opaque_ = big_endian::read_uint32(header_buffer_, 12);
    packet.cas_ = big_endian::read_uint64(header_buffer_, 16);

    std::size_t ext_len = std::to_integer<std::size_t>(header_buffer_[5]);
    std::size_t key_len = big_endian::read_uint16(header_buffer_, 2);
    std::size_t frames_len = 0;

    switch (magic) {
        case mcbp::command_magic::request_ext:
        case mcbp::command_magic::response_ext:
            key_len = std::to_integer<std::size_t>(header_buffer_[3]);
            frames_len = std::to_integer<std::size_t>(header_buffer_[2]);
            break;

        default:
            break;
    }

    if (frames_len + ext_len + key_len != body_len) {
        LOG_DEBUG("frames_len ({}) + ext_len ({}) + key_len ({}) != body_len ({})", frames_len, ext_len, key_len, body_len);
        return { {}, {}, errc::network::protocol_error };
    }
    std::size_t value_len = body_len - frames_len + ext_len + key_len;

    if (frames_len > 0) {
        std::size_t frame_offset = 0;

        while (frame_offset < frames_len) {
            std::byte frame_header = body[frame_offset];
            ++frame_offset;

            auto frame_type = static_cast<mcbp::frame_type>((frame_header & std::byte{ 0xf0 }) >> 4);
            if (frame_type == 15) {
                frame_type += static_cast<mcbp::frame_type>(body[frame_offset]);
                ++frame_offset;
            }

            auto frame_len = static_cast<std::size_t>(frame_header & std::byte{ 0x0f });
            if (frame_len == 15) {
                frame_len += static_cast<mcbp::frame_type>(body[frame_offset]);
                ++frame_offset;
            }

            switch (magic) {
                case mcbp::command_magic::request_ext:
                    if (frame_type == mcbp::request_barrier && frame_len == 0) {
                        packet.barrier_frame_ = mcbp::barrier_frame{};
                    } else if (frame_type == mcbp::request_sync_durability && (frame_len == 1 || frame_len == 3)) {
                        packet.durability_level_frame_ = mcbp::durability_level_frame{ mcbp::durability_level{ body[frame_offset] } };
                        if (frame_len == 3) {
                            packet.durability_timeout_frame_ = mcbp::durability_timeout_frame{ std::chrono::milliseconds{
                              big_endian::read_uint16(body, frame_offset + 1) } };
                        } else {
                            // We follow the semantic that duplicate frames overwrite previous ones, since the timeout frame is 'virtual' to
                            // us, we need to clear it in case this is a duplicate frame.
                            packet.durability_timeout_frame_.reset();
                        }
                    } else if (frame_type == mcbp::request_stream_id && frame_len == 2) {
                        packet.stream_id_frame_ = mcbp::stream_id_frame{ big_endian::read_uint16(body, frame_offset) };
                    } else if (frame_type == mcbp::request_open_tracing && frame_len > 0) {
                        packet.open_tracing_frame_ = mcbp::open_tracing_frame{ big_endian::read(body, frame_offset, frame_len) };
                    } else if (frame_type == mcbp::request_preserve_expiry && frame_len == 0) {
                        packet.preserve_expiry_frame_ = mcbp::preserve_expiry_frame{};
                    } else if (frame_type == mcbp::request_user_impersonation && frame_len > 0) {
                        packet.user_impersonation_frame_ =
                          mcbp::user_impersonation_frame{ big_endian::read(body, frame_offset, frame_len) };
                    } else {
                        // If we don't understand this frame type, we record it as an UnsupportedFrame (as opposed to dropping it blindly)
                        packet.unsupported_frames_.emplace_back(
                          mcbp::unsupported_frame{ frame_type, big_endian::read(body, frame_offset, frame_len) });
                    }
                    break;

                case mcbp::command_magic::response_ext:
                    if (frame_type == mcbp::response_server_duration && frame_len == 2) {
                        packet.server_duration_frame_ =
                          mcbp::server_duration_frame{ mcbp::decode_server_duration(big_endian::read_uint16(body, frame_offset)) };
                    } else if (frame_type == mcbp::response_read_units && frame_len == 2) {
                        packet.read_units_frame_ = mcbp::read_units_frame{ big_endian::read_uint16(body, frame_offset) };
                    } else if (frame_type == mcbp::response_write_units && frame_len == 2) {
                        packet.write_units_frame_ = mcbp::write_units_frame{ big_endian::read_uint16(body, frame_offset) };
                    } else {
                        // If we don't understand this frame type, we record it as an UnsupportedFrame (as opposed to dropping it blindly)
                        packet.unsupported_frames_.emplace_back(
                          mcbp::unsupported_frame{ frame_type, big_endian::read(body, frame_offset, frame_len) });
                    }
                    break;

                default:
                    LOG_DEBUG("got unexpected magic when decoding frames");
                    return { {}, {}, errc::network::protocol_error };
            }
            frame_offset += frame_len;
        }
    }

    if (ext_len > 0) {
        packet.extras_ = big_endian::read(body, frames_len, ext_len);
    }
    if (key_len > 0) {
        packet.key_ = big_endian::read(body, frames_len + ext_len, key_len);
    }
    if (value_len > 0) {
        packet.value_ = big_endian::read(body, frames_len + ext_len + key_len, body_len);
    }

    if (collections_enabled_) {
        if (packet.command_ == mcbp::command_code::observe) {
            // While it's possible that the Observe operation is in fact supported with collections
            // enabled, we don't currently implement that operation for simplicity, as the key is
            // actually hidden away in the value data instead of the usual key data.
            LOG_DEBUG("the observe operation is not supported with collections enabled");
            return { {}, {}, errc::common::feature_not_available };
        }
        if (key_len > 0 && supports_collection_id(packet.command_)) {
            auto [id, remaining] = core::utils::decode_unsigned_leb128<std::uint32_t>(packet.key_, core::utils::leb_128_no_throw{});
            if (remaining.empty()) {
                LOG_DEBUG("unable to decode collection id");
                return { {}, {}, errc::network::protocol_error };
            }
            packet.collection_id_ = id;
            packet.key_ = std::move(remaining);
        }
    }

    return { packet, header_buffer_.size() + body_len, {} };
}

std::error_code
mcbp_connection::close()
{
    // FIXME: implement
    return {};
}

void
mcbp_connection::release()
{
    // FIXME: implement
}

void
mcbp_connection::enable_feature(mcbp::hello_feature feature)
{
    enabled_features_.insert(feature);
    if (feature == mcbp::hello_feature::collections) {
        collections_enabled_ = true;
    }
}

bool
mcbp_connection::is_feature_enabled(mcbp::hello_feature feature)
{
    return enabled_features_.count(feature) > 0;
}
} // namespace couchbase::new_core::impl
