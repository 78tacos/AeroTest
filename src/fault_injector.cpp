// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/fault_injector.hpp"

namespace aerotest {

Status FaultInjector::arm(FaultSpec spec) {
    // TODO(step-1): task T5 in docs/step-1.md.
    // Reject a bad `remaining` before storing anything.
    (void)spec;
    return Status::not_implemented("FaultInjector::arm");
}

FaultEffect FaultInjector::apply_tx(Message& message) {
    // TODO(step-1): task T5 in docs/step-1.md.
    // With nothing armed, pass the frame through: Ok, deliver, no extra delay,
    // `message` unchanged. Otherwise apply the first matching spec only.
    (void)message;
    return FaultEffect{};
}

} // namespace aerotest
