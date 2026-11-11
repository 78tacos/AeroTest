// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/bus.hpp"

#include <string>

namespace aerotest {

/// Linux SocketCAN bus. The constructor stores the interface name and does not
/// open a socket. `name()` and `is_open()` are done. The rest is task T7.
///
/// `is_open()` is true only when `socket_fd_` is a live descriptor (>= 0).
/// There is no second flag to keep in sync.
class SocketCanBus final : public Bus {
  public:
    /// `interface_name` is a Linux name such as "vcan0" or "can0".
    explicit SocketCanBus(std::string interface_name);
    ~SocketCanBus() override;

    SocketCanBus(const SocketCanBus&) = delete;
    SocketCanBus& operator=(const SocketCanBus&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override { return interface_name_; }
    [[nodiscard]] bool is_open() const noexcept override { return socket_fd_ >= 0; }

    Status open() override;
    Status close() override;
    Status send(const Message& message) override;
    Status receive(Message& out, std::chrono::milliseconds timeout) override;

  private:
    std::string interface_name_;
    int socket_fd_{-1};
};

} // namespace aerotest
