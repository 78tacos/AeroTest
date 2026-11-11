// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/subsystem.hpp"

#include <cstdint>
#include <string_view>

namespace aerotest {

/// Request id. Decimal 512 in a scenario file.
inline constexpr std::uint32_t kEngineRequestId = 0x200;
/// Reply id. Decimal 513 in a scenario file.
inline constexpr std::uint32_t kEngineReplyId = 0x201;

/// Fake engine node.
///
/// `name()` and the two published values are done. `on_frame` is task T3.
///
/// A request is any frame whose id is `kEngineRequestId`. The reply id is
/// `kEngineReplyId` and the data is four bytes, little-endian uint16, rpm
/// then coolant:
///
/// - rpm 2400     -> bytes 96, 9
/// - coolant 90 C -> bytes 90, 0
///
/// So the default reply data is `{96, 9, 90, 0}`.
class EngineSubsystem final : public Subsystem {
  public:
    std::uint16_t rpm{2400};
    std::uint16_t coolant_c{90};

    [[nodiscard]] std::string_view name() const noexcept override;

    Status on_frame(Bus& bus, const Message& received) override;
};

} // namespace aerotest
