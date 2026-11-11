// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/scenario.hpp"
#include "aerotest/status.hpp"

#include <filesystem>
#include <string_view>

namespace aerotest {

/// Turns scenario text into a `Scenario`.
///
/// On failure, leave `out` exactly as the caller passed it. A half-filled
/// scenario is worse than a clean error: the next step would run the wrong test.
class ScenarioParser {
  public:
    virtual ~ScenarioParser() = default;

    ScenarioParser(const ScenarioParser&) = delete;
    ScenarioParser& operator=(const ScenarioParser&) = delete;

    virtual Status parse(std::string_view yaml, Scenario& out) = 0;

  protected:
    ScenarioParser() = default;
};

/// `yaml-cpp` implementation. The header does not include yaml-cpp; only the
/// `.cpp` does, and the library links it privately.
///
/// `backend()` is done. `parse` and `parse_file` are task T1 in docs/step-2.md.
///
/// File shape, `scenarios/air_data_smoke.yaml`:
///
/// ```
/// name: air-data-smoke          # required, non-empty
/// description: ...              # optional, default empty
/// bus: loopback                 # optional, default loopback; empty string is invalid
/// subsystems:                   # optional. Only "air-data" and "engine"
///   - air-data
/// steps:                        # required sequence. Empty is legal
///   - name: request             # optional
///     requirement: REQ-AD-001   # optional
///     send:
///       id: 256                 # required, non-negative integer
///       data: []                # optional bytes, each 0..255, default empty
///   - name: indicated
///     requirement: REQ-AD-001
///     expect:
///       id: 257
///       data: [100, 0, 50, 0]   # required unless absent is true
///       timeout_ms: 20          # required, greater than zero
///       absent: false           # optional
///   - name: drop the reply
///     inject:
///       kind: drop              # drop, corrupt, delay, or error
///       match_id: 513           # optional
///       byte_index: 0           # optional
///       xor_mask: 255           # optional
///       delay_ms: 0             # optional
///       remaining: 1            # optional, same rules as FaultSpec
///   - wait:
///       ms: 0                   # required, zero or more
/// ```
///
/// A step has exactly one of `send`, `expect`, `inject`, `wait`.
class YamlScenarioParser final : public ScenarioParser {
  public:
    /// "yaml-cpp" when this binary is linked to that library. This is not a parse.
    [[nodiscard]] const char* backend() const;

    Status parse(std::string_view yaml, Scenario& out) override;

    /// Read `path`, then `parse`. A missing or unreadable file is `Error::Io`
    /// and leaves `out` unchanged.
    Status parse_file(const std::filesystem::path& path, Scenario& out);
};

} // namespace aerotest
