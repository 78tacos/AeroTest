// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/fault_injector.hpp"
#include "aerotest/results_writer.hpp"
#include "aerotest/scenario.hpp"
#include "aerotest/subsystem.hpp"
#include "aerotest/test_runner.hpp"

#include <cstddef>
#include <vector>

namespace aerotest {

/// Plays a `Scenario` on a bus the caller already opened.
///
/// The runner does not construct a bus, and it does not call `open` or `close`.
/// `add_subsystem` is done. `run` is task T4 in docs/step-2.md.
///
/// Before any step:
///
/// - If `scenario.bus` is not `bus.name()`, return InvalidArgument and leave
///   `out` unchanged. Do this even when the bus is closed.
/// - If the bus is not open, return NotOpen and leave `out` unchanged.
/// - If a name in `scenario.subsystems` was not `add_subsystem`'d, return
///   InvalidArgument and leave `out` unchanged.
/// - Call `bus.set_fault_injector` with the injector passed to the constructor.
///
/// Then each step appends one `CheckResult` and the run continues. `run`
/// returns Ok when the steps finished, including when a check failed. The
/// caller reads `out.passed()`. A step status other than Ok is a failed check,
/// not a reason to abort the rest.
///
/// - Send: `bus.send` a copy of `message`. On Ok, call `on_frame` for each
///   subsystem named in the scenario, in that order, with the same message.
///   The first `on_frame` that fails ends the dispatch for this step. A failed
///   send does not call subsystems.
/// - Expect, `absent` false: `checks.expect_message`, then copy the returned
///   check, set `requirement_id` from the step, and store that copy. Do not
///   also walk `report().checks` or the check is recorded twice. Step 1 leaves
///   the id empty; this runner is what fills it.
/// - Expect, `absent` true: `bus.receive`. Timeout is a pass. A frame is a
///   fail. Do not call `expect_message` — a timeout is a failure there.
/// - Inject: `faults.arm(step.fault)` and record that status.
/// - Wait: `sleep_for(step.wait)`, then a passing check. This is the only sleep.
///   Loopback `expect` does not block. A Delay fault needs a wait before the expect.
///
/// Switch on `StepKind` with no default that returns a value, so a new kind
/// fails the build until it is handled (same idea as `to_string` in step 1).
class ScenarioRunner {
  public:
    explicit ScenarioRunner(Bus& bus, TestRunner& checks, FaultInjector& faults);

    ScenarioRunner(const ScenarioRunner&) = delete;
    ScenarioRunner& operator=(const ScenarioRunner&) = delete;

    /// Non-owning. The caller keeps `subsystem` alive for the run.
    void add_subsystem(Subsystem& subsystem) { subsystems_.push_back(&subsystem); }

    [[nodiscard]] std::size_t subsystem_count() const noexcept { return subsystems_.size(); }

    Status run(const Scenario& scenario, ScenarioResult& out);

  private:
    Bus& bus_;
    TestRunner& checks_;
    FaultInjector& faults_;
    std::vector<Subsystem*> subsystems_;
};

} // namespace aerotest
