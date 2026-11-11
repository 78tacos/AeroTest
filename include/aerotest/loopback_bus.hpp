// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/bus.hpp"

#include <chrono>
#include <cstddef>
#include <deque>

namespace aerotest {

/// In-memory bus. No sockets and no sleeping.
/// `name()`, `is_open()`, and `queued()` are done. `open`, `close`, `send`,
/// and `receive` are tasks T3, T4, and the loopback half of T5.
class LoopbackBus final : public Bus {
  public:
    LoopbackBus() = default;

    [[nodiscard]] std::string_view name() const noexcept override { return "loopback"; }
    [[nodiscard]] bool is_open() const noexcept override { return open_; }
    [[nodiscard]] std::size_t queued() const noexcept { return queue_.size(); }

    Status open() override;
    Status close() override;
    Status send(const Message& message) override;
    Status receive(Message& out, std::chrono::milliseconds timeout) override;

  private:
    /// A frame waiting for `receive`. `ready_at` in the past (including the
    /// clock's epoch) means the frame can be popped now. A Delay fault sets
    /// `ready_at` in the future. `receive` must not sleep; it checks the clock once.
    struct Queued {
        Message message;
        std::chrono::steady_clock::time_point ready_at{};
    };

    bool open_{false};
    std::deque<Queued> queue_;
};

} // namespace aerotest
