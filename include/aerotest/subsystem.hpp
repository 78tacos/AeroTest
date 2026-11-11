// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/bus.hpp"

#include <string_view>

namespace aerotest {

/// A synthetic node that answers frames on a bus it does not own.
///
/// The loopback is one queue, not a publish/subscribe fabric. The scenario
/// runner calls `on_frame` after a successful `send`, and the node replies
/// with its own `bus.send`. Nothing here opens a socket.
class Subsystem {
  public:
    virtual ~Subsystem() = default;

    Subsystem(const Subsystem&) = delete;
    Subsystem& operator=(const Subsystem&) = delete;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// `received` is the frame the runner just sent. Do not modify it.
    /// An id this node does not own returns Ok and sends nothing.
    /// The node's own request, on a closed bus, returns NotOpen and sends nothing.
    virtual Status on_frame(Bus& bus, const Message& received) = 0;

  protected:
    Subsystem() = default;
};

} // namespace aerotest
