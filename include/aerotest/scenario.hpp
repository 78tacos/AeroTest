// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/fault_injector.hpp"
#include "aerotest/message.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace aerotest {

/// One line in a scenario file. Exactly one kind is set.
///
/// The YAML keys are `send`, `expect`, `inject`, and `wait`. Decimal integers
/// only: `256` is CAN id `0x100`. There is no hex in the file format, so the
/// parser does not grow a second number syntax.
enum class StepKind {
    Send,   ///< transmit `message`, then show it to each named subsystem
    Expect, ///< compare a received frame, or require that none arrives
    Inject, ///< arm `fault` on the runner's injector
    Wait    ///< sleep for `wait`. This is the only step that blocks
};

/// One step, already parsed. Fields that do not belong to `kind` stay defaulted.
struct ScenarioStep {
    StepKind kind{StepKind::Send};
    /// Empty in the file means the kind name: "send", "expect", "inject", "wait".
    std::string name;
    /// Copied onto the check the runner records. Empty when the file omits it.
    std::string requirement_id;
    /// `send` and `expect`. Standard data frames only. Id kind and remote stay default.
    Message message;
    /// `expect` only. True means a Timeout is a pass and a received frame is a fail.
    bool absent{false};
    /// `expect` only. Must be greater than zero.
    std::chrono::milliseconds timeout{0};
    /// `inject` only.
    FaultSpec fault;
    /// `wait` only. Zero is legal and does not sleep.
    std::chrono::milliseconds wait{0};
};

/// A whole scenario. The runner does not open a bus from `bus`: the caller
/// passes a bus whose `name()` equals this string.
struct Scenario {
    std::string name;
    std::string description;
    std::string bus{"loopback"};
    /// Closed set for step 2: "air-data" and "engine". Order is dispatch order.
    std::vector<std::string> subsystems;
    std::vector<ScenarioStep> steps;
};

} // namespace aerotest
