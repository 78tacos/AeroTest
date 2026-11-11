// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/message.hpp"

#include <chrono>
#include <string_view>

namespace aerotest {

class FaultInjector;

/// Something that can open, send one frame, and receive one frame.
///
/// `LoopbackBus` is an in-memory stand-in for tests. `SocketCanBus` talks to a
/// Linux CAN interface such as vcan0. A later 1553 driver should follow this
/// shape (open, send, receive, close) with its own frame type. It should not
/// pretend a 1553 word is a `Message`.
///
/// Buses are not copyable. Step 1 is single-threaded: one runner, one bus, no
/// internal mutex.
class Bus {
  public:
    virtual ~Bus() = default;

    Bus(const Bus&) = delete;
    Bus& operator=(const Bus&) = delete;
    Bus(Bus&&) = delete;
    Bus& operator=(Bus&&) = delete;

    /// Storage that lives at least as long as the bus.
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual bool is_open() const noexcept = 0;

    virtual Status open() = 0;
    virtual Status close() = 0;

    /// Queue or transmit a frame. Does not take ownership of `message` and does
    /// not modify it. Implementations copy first, then validate and apply faults.
    virtual Status send(const Message& message) = 0;

    /// Take one ready frame.
    /// On success, write it to `out` and stamp `out.timestamp`.
    /// On failure, leave `out` unchanged.
    /// A negative `timeout` is InvalidArgument once the bus is open.
    /// `LoopbackBus` does not block: an empty queue or a frame that is not ready
    /// yet returns Timeout immediately. `SocketCanBus` is where `timeout` waits.
    virtual Status receive(Message& out, std::chrono::milliseconds timeout) = 0;

    /// Non-owning. The caller keeps the injector alive. Pass nullptr to detach.
    /// Storing the pointer is already implemented. Honoring it inside `send` is task T5.
    void set_fault_injector(FaultInjector* injector) noexcept { injector_ = injector; }

    [[nodiscard]] FaultInjector* fault_injector() const noexcept { return injector_; }

  protected:
    Bus() = default;

  private:
    FaultInjector* injector_{nullptr};
};

} // namespace aerotest
