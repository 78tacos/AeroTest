// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/message.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace aerotest {

/// What to do to one transmitted frame. The bus applies this. The injector decides.
enum class FaultKind {
    /// `send` returns Ok and the frame is not queued.
    Drop,
    /// XOR one data byte, then deliver the damaged copy.
    Corrupt,
    /// Deliver the frame only after `delay`. `send` itself does not block.
    Delay,
    /// `send` returns FaultInjected and the frame is not queued.
    Error
};

/// One armed fault. Fields are public so a test can fill them in any order.
struct FaultSpec {
    FaultKind kind{FaultKind::Drop};
    /// Empty matches every id. Set it to limit the fault to one id.
    std::optional<std::uint32_t> match_id;
    std::size_t byte_index{0};
    std::uint8_t xor_mask{0xFF};
    std::chrono::milliseconds delay{0};
    /// How many matching frames this spec still applies to.
    /// 1 or more counts down. -1 lasts until `clear()`. 0 and values below -1
    /// are rejected by `arm`.
    int remaining{1};
};

/// What `apply_tx` tells the bus to do with the copy it passed in.
struct FaultEffect {
    Error code{Error::NotImplemented};
    /// When false, the bus must not queue the frame.
    bool deliver{false};
    std::chrono::milliseconds extra_delay{0};

    [[nodiscard]] bool ok() const noexcept { return code == Error::Ok; }
};

/// Holds fault specs and edits a frame the bus is about to send.
///
/// The bus owns none of this. The caller creates the injector, passes its
/// address to `Bus::set_fault_injector`, and keeps it alive.
/// `apply_tx` may edit `message`. Callers that must keep the original pass a copy.
class FaultInjector {
  public:
    /// Store `spec` until it is used up or `clear()` runs. Task T5.
    Status arm(FaultSpec spec);

    void clear() noexcept { armed_.clear(); }

    [[nodiscard]] std::size_t pending() const noexcept { return armed_.size(); }

    /// First matching spec wins. A spec that does not match stays armed and is
    /// not counted down. Task T5. The stub returns NotImplemented and delivers nothing.
    FaultEffect apply_tx(Message& message);

  private:
    std::vector<FaultSpec> armed_;
};

} // namespace aerotest
