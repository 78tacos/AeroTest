// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/status.hpp"

#include <chrono>
#include <cstdint>
#include <vector>

namespace aerotest {

/// Classical CAN identifier width. Step 1 does not model CAN FD.
enum class IdKind {
    Standard, ///< 11-bit id, 0 .. 0x7FF
    Extended  ///< 29-bit id, 0 .. 0x1FFFFFFF
};

/// One classical CAN frame.
///
/// This is a CAN frame, not a universal avionics frame. A later MIL-STD-1553
/// command word is a different type. Do not fold 1553 or ARINC 429 into these
/// fields. CAN FD (more than 8 data bytes) is out of scope for step 1.
struct Message {
    std::uint32_t id{0};
    IdKind id_kind{IdKind::Standard};
    std::vector<std::uint8_t> data;
    bool remote_request{false};
    /// Set by the bus on receive, from `steady_clock`. Zero means "not stamped".
    /// The test runner does not compare this field.
    std::chrono::nanoseconds timestamp{0};

    /// Classical CAN checks. Task T2. The stub returns NotImplemented.
    ///
    /// A frame is legal when all of these hold:
    /// - Standard ids are in 0 .. 0x7FF.
    /// - Extended ids are in 0 .. 0x1FFFFFFF.
    /// - `data.size()` is at most 8.
    /// - A remote frame has an empty `data` vector.
    /// An id of 0 and a data length of 0 are legal.
    [[nodiscard]] Status validate() const;
};

} // namespace aerotest
