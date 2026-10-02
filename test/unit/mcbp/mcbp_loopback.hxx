/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2026-Present Couchbase, Inc.
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

#include "core/protocol/client_opcode.hxx"

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/post.hpp>
#include <asio/write.hpp>

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace couchbase::test
{
/**
 * A scripted key-value endpoint on 127.0.0.1 with its own io thread.
 *
 * It answers the requests an mcbp_session sends to bootstrap with PLAIN credentials and records
 * every request frame it receives. Responses:
 *
 * - hello: success, no features, so the session negotiates neither xerror (no get_error_map) nor
 *   collections (keys go on the wire unprefixed).
 * - sasl_list_mechs: "PLAIN". sasl_auth: success.
 * - select_bucket: success.
 * - get_cluster_config: a one-node, one-vbucket configuration naming this endpoint. While
 *   hold_cluster_config() is in effect the reply is withheld until release_cluster_config().
 *
 * Any other request is recorded and left unanswered.
 */
class mcbp_loopback
{
public:
  using protocol_opcode = core::protocol::client_opcode;

  struct frame {
    protocol_opcode opcode;
    std::string key;
  };

  explicit mcbp_loopback(std::string bucket_name = {})
    : bucket_name_{ std::move(bucket_name) }
  {
    accept();
    thread_ = std::thread([this]() {
      io_.run();
    });
  }

  mcbp_loopback(const mcbp_loopback&) = delete;
  mcbp_loopback(mcbp_loopback&&) = delete;
  auto operator=(const mcbp_loopback&) -> mcbp_loopback& = delete;
  auto operator=(mcbp_loopback&&) -> mcbp_loopback& = delete;

  ~mcbp_loopback()
  {
    asio::post(io_, [this]() {
      std::error_code ignored;
      acceptor_.close(ignored);
      for (const auto& c : connections_) {
        c->socket.close(ignored);
      }
      connections_.clear();
      pending_configs_.clear();
    });
    guard_.reset();
    thread_.join();
  }

  [[nodiscard]] auto port() const -> std::uint16_t
  {
    return port_;
  }

  void hold_cluster_config()
  {
    asio::post(io_, [this]() {
      hold_config_ = true;
    });
  }

  void release_cluster_config()
  {
    asio::post(io_, [this]() {
      hold_config_ = false;
      for (auto& [weak, opaque] : pending_configs_) {
        if (auto c = weak.lock(); c) {
          reply_cluster_config(c, opaque);
        }
      }
      pending_configs_.clear();
    });
  }

  [[nodiscard]] auto accepted() const -> std::size_t
  {
    const std::scoped_lock lock(mutex_);
    return accepted_;
  }

  [[nodiscard]] auto frames() const -> std::vector<frame>
  {
    const std::scoped_lock lock(mutex_);
    return frames_;
  }

  [[nodiscard]] auto received(protocol_opcode opcode) const -> std::size_t
  {
    const std::scoped_lock lock(mutex_);
    std::size_t n{ 0 };
    for (const auto& f : frames_) {
      n += (f.opcode == opcode) ? 1 : 0;
    }
    return n;
  }

  [[nodiscard]] auto received_key(const std::string& key) const -> bool
  {
    const std::scoped_lock lock(mutex_);
    for (const auto& f : frames_) {
      if (f.key == key) {
        return true;
      }
    }
    return false;
  }

  /** Blocks until predicate holds or timeout passes; returns the predicate's last value. */
  auto wait_until(const std::function<bool()>& predicate, std::chrono::milliseconds timeout) const
    -> bool
  {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [&]() {
      lock.unlock();
      auto done = predicate();
      lock.lock();
      return done;
    });
  }

private:
  using opaque_bytes = std::array<std::uint8_t, 4>;

  struct connection {
    explicit connection(asio::io_context& io)
      : socket{ io }
    {
    }

    asio::ip::tcp::socket socket;
    std::array<std::uint8_t, 16384> chunk{};
    std::vector<std::uint8_t> input{};
    std::deque<std::vector<std::uint8_t>> outbox{};
  };

  static constexpr std::size_t header_size{ 24 };

  void accept()
  {
    auto c = std::make_shared<connection>(io_);
    acceptor_.async_accept(c->socket, [this, c](std::error_code ec) {
      if (ec) {
        return;
      }
      connections_.push_back(c);
      {
        const std::scoped_lock lock(mutex_);
        ++accepted_;
      }
      changed_.notify_all();
      read(c);
      accept();
    });
  }

  void read(const std::shared_ptr<connection>& c)
  {
    c->socket.async_read_some(asio::buffer(c->chunk), [this, c](std::error_code ec, std::size_t n) {
      if (ec) {
        return;
      }
      c->input.insert(c->input.end(), c->chunk.begin(), c->chunk.begin() + n);
      consume(c);
      read(c);
    });
  }

  void consume(const std::shared_ptr<connection>& c)
  {
    for (;;) {
      if (c->input.size() < header_size) {
        return;
      }
      const auto* h = c->input.data();
      const std::size_t body_size = (std::size_t{ h[8] } << 24U) | (std::size_t{ h[9] } << 16U) |
                                    (std::size_t{ h[10] } << 8U) | std::size_t{ h[11] };
      if (c->input.size() < header_size + body_size) {
        return;
      }
      // The alternative request magic carries framing extras and a one-byte key length.
      const bool alt = h[0] == 0x08;
      const std::size_t framing_size = alt ? h[2] : 0;
      const std::size_t key_size = alt ? h[3] : ((std::size_t{ h[2] } << 8U) | h[3]);
      const std::size_t extras_size = h[4];
      const auto opcode = static_cast<protocol_opcode>(h[1]);
      const opaque_bytes opaque{ h[12], h[13], h[14], h[15] };
      const auto* key = h + header_size + framing_size + extras_size;
      {
        const std::scoped_lock lock(mutex_);
        frames_.push_back({ opcode, std::string(key, key + key_size) });
      }
      changed_.notify_all();
      c->input.erase(c->input.begin(),
                     c->input.begin() + static_cast<std::ptrdiff_t>(header_size + body_size));
      respond(c, opcode, opaque);
    }
  }

  void respond(const std::shared_ptr<connection>& c, protocol_opcode opcode, opaque_bytes opaque)
  {
    switch (opcode) {
      case protocol_opcode::hello:
      case protocol_opcode::sasl_auth:
      case protocol_opcode::select_bucket:
        return send(c, opcode, opaque, {}, 0);
      case protocol_opcode::sasl_list_mechs:
        return send(c, opcode, opaque, "PLAIN", 0);
      case protocol_opcode::get_cluster_config:
        if (hold_config_) {
          pending_configs_.emplace_back(c, opaque);
          return;
        }
        return reply_cluster_config(c, opaque);
      default:
        return;
    }
  }

  void reply_cluster_config(const std::shared_ptr<connection>& c, opaque_bytes opaque)
  {
    const auto address = "127.0.0.1:" + std::to_string(port_);
    std::string config = R"({"rev":1,"nodeLocator":"vbucket",)";
    if (!bucket_name_.empty()) {
      config += R"("name":")" + bucket_name_ + R"(",)";
    }
    config += R"("nodesExt":[{"hostname":"127.0.0.1","thisNode":true,"services":{"kv":)" +
              std::to_string(port_) + R"(}}],)" +
              R"("vBucketServerMap":{"numReplicas":0,"serverList":[")" + address +
              R"("],"vBucketMap":[[0]]}})";
    send(c, protocol_opcode::get_cluster_config, opaque, config, /* JSON datatype */ 0x01);
  }

  void send(const std::shared_ptr<connection>& c,
            protocol_opcode opcode,
            opaque_bytes opaque,
            const std::string& body,
            std::uint8_t datatype)
  {
    std::vector<std::uint8_t> packet(header_size, 0);
    packet[0] = 0x81; // client_response
    packet[1] = static_cast<std::uint8_t>(opcode);
    packet[5] = datatype;
    const auto size = static_cast<std::uint32_t>(body.size());
    packet[8] = static_cast<std::uint8_t>(size >> 24U);
    packet[9] = static_cast<std::uint8_t>(size >> 16U);
    packet[10] = static_cast<std::uint8_t>(size >> 8U);
    packet[11] = static_cast<std::uint8_t>(size);
    std::copy(opaque.begin(), opaque.end(), packet.begin() + 12);
    packet.insert(packet.end(), body.begin(), body.end());
    c->outbox.push_back(std::move(packet));
    if (c->outbox.size() == 1) {
      write(c);
    }
  }

  void write(const std::shared_ptr<connection>& c)
  {
    asio::async_write(
      c->socket, asio::buffer(c->outbox.front()), [this, c](std::error_code ec, std::size_t) {
        if (ec) {
          return;
        }
        c->outbox.pop_front();
        if (!c->outbox.empty()) {
          write(c);
        }
      });
  }

  std::string bucket_name_;
  asio::io_context io_{};
  asio::executor_work_guard<asio::io_context::executor_type> guard_{ io_.get_executor() };
  asio::ip::tcp::acceptor acceptor_{ io_,
                                     asio::ip::tcp::endpoint{ asio::ip::make_address("127.0.0.1"),
                                                              0 } };
  std::uint16_t port_{ acceptor_.local_endpoint().port() };
  std::thread thread_{};

  // Touched only on the io thread.
  std::vector<std::shared_ptr<connection>> connections_{};
  std::vector<std::pair<std::weak_ptr<connection>, opaque_bytes>> pending_configs_{};
  bool hold_config_{ false };

  mutable std::mutex mutex_{};
  mutable std::condition_variable changed_{};
  std::size_t accepted_{ 0 };
  std::vector<frame> frames_{};
};
} // namespace couchbase::test
