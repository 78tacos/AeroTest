// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/loopback_bus.hpp"

namespace aerotest {

Status LoopbackBus::open() {
    // TODO(step-1): task T3 in docs/step-1.md.
    return Status::not_implemented("LoopbackBus::open");
}

Status LoopbackBus::close() {
    // TODO(step-1): task T3 in docs/step-1.md.
    return Status::not_implemented("LoopbackBus::close");
}

Status LoopbackBus::send(const Message& message) {
    // TODO(step-1): tasks T4 and T5 in docs/step-1.md.
    // Copy `message` first. Validate the copy. Then ask the fault injector, if one
    // is attached. Push a Queued onto the back of queue_ only when the effect says
    // to deliver. Do not modify `message`.
    (void)message;
    return Status::not_implemented("LoopbackBus::send");
}

Status LoopbackBus::receive(Message& out, std::chrono::milliseconds timeout) {
    // TODO(step-1): task T4 in docs/step-1.md.
    // Do not block and do not sleep. On success, pop the front frame, stamp
    // `timestamp` from steady_clock, and write `out`. On failure, leave `out` alone.
    (void)out;
    (void)timeout;
    return Status::not_implemented("LoopbackBus::receive");
}

} // namespace aerotest
