// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/subsystem.hpp"

#include <cstdint>
#include <string_view>

namespace aerotest {

/// Request id. Decimal 256 in a scenario file.
inline constexpr std::uint32_t kAirDataRequestId = 0x100;
/// Reply id. Decimal 257 in a scenario file.
inline constexpr std::uint32_t kAirDataReplyId = 0x101;

/// Fake air-data node.
///
/// `name()` and the two published values are done. `on_frame` is task T3.
///
/// A request is any frame whose id is `kAirDataRequestId`. The payload is
/// ignored. The reply id is `kAirDataReplyId` and the data is four bytes,
/// little-endian uint16, altitude then airspeed:
///
/// - altitude 100 -> bytes 100, 0
/// - airspeed 50  -> bytes 50, 0
///
/// So the default reply data is `{100, 0, 50, 0}`.
class AirDataSubsystem final : public Subsystem {
  public:
    std::uint16_t altitude_ft{100};
    std::uint16_t airspeed_kt{50};

    [[nodiscard]] std::string_view name() const noexcept override;

    Status on_frame(Bus& bus, const Message& received) override;
};

} // namespace aerotest
