// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/socketcan_bus.hpp"

#include <utility>

namespace aerotest {

SocketCanBus::SocketCanBus(std::string interface_name)
    : interface_name_(std::move(interface_name)) {}

SocketCanBus::~SocketCanBus() {
    // TODO(step-1): task T7 in docs/step-1.md.
    // When socket_fd_ is open, close it and set it back to -1.
    // -1 means there is no descriptor yet, so this empty body is correct until
    // open() starts creating sockets.
}

Status SocketCanBus::open() {
    // TODO(step-1): task T7 in docs/step-1.md.
    return Status::not_implemented("SocketCanBus::open");
}

Status SocketCanBus::close() {
    // TODO(step-1): task T7 in docs/step-1.md.
    return Status::not_implemented("SocketCanBus::close");
}

Status SocketCanBus::send(const Message& message) {
    // TODO(step-1): task T7 in docs/step-1.md.
    (void)message;
    return Status::not_implemented("SocketCanBus::send");
}

Status SocketCanBus::receive(Message& out, std::chrono::milliseconds timeout) {
    // TODO(step-1): task T7 in docs/step-1.md.
    (void)out;
    (void)timeout;
    return Status::not_implemented("SocketCanBus::receive");
}

} // namespace aerotest
