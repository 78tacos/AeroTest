// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/scenario_runner.hpp"

namespace aerotest {

ScenarioRunner::ScenarioRunner(Bus& bus, TestRunner& checks, FaultInjector& faults)
    : bus_(bus), checks_(checks), faults_(faults) {}

Status ScenarioRunner::run(const Scenario& scenario, ScenarioResult& out) {
    // TODO(step-2): task T4 in docs/step-2.md.
    // Refuse a mismatched bus name, a closed bus, or an unknown subsystem
    // before touching `out`. Then play each step and record one check.
    (void)scenario;
    (void)out;
    // Referenced so Clang's -Wunused-private-field stays quiet until T4 uses them.
    (void)bus_;
    (void)checks_;
    (void)faults_;
    return Status::not_implemented("ScenarioRunner::run");
}

} // namespace aerotest
