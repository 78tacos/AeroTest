// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/engine_subsystem.hpp"

namespace aerotest {

std::string_view EngineSubsystem::name() const noexcept {
    return "engine";
}

Status EngineSubsystem::on_frame(Bus& bus, const Message& received) {
    // TODO(step-2): task T3 in docs/step-2.md.
    // Reply to kEngineRequestId with four little-endian bytes. Any other id
    // returns Ok and sends nothing. Do not modify `received`.
    (void)bus;
    (void)received;
    return Status::not_implemented("EngineSubsystem::on_frame");
}

} // namespace aerotest
