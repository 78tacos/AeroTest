// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/air_data_subsystem.hpp"
#include "aerotest/engine_subsystem.hpp"
#include "aerotest/loopback_bus.hpp"
#include "aerotest/scenario_runner.hpp"

#include "support.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

using aerotest::AirDataSubsystem;
using aerotest::EngineSubsystem;
using aerotest::Error;
using aerotest::FaultInjector;
using aerotest::FaultKind;
using aerotest::kAirDataReplyId;
using aerotest::kAirDataRequestId;
using aerotest::kEngineReplyId;
using aerotest::kEngineRequestId;
using aerotest::LoopbackBus;
using aerotest::Scenario;
using aerotest::ScenarioResult;
using aerotest::ScenarioRunner;
using aerotest::ScenarioStep;
using aerotest::StepKind;
using aerotest::TestRunner;
using aerotest::test::bytes;

namespace {

ScenarioStep send_step(std::uint32_t id, std::string requirement) {
    ScenarioStep step;
    step.kind = StepKind::Send;
    step.name = "send";
    step.requirement_id = std::move(requirement);
    step.message.id = id;
    return step;
}

} // namespace

TEST(ScenarioRunner, SlotStartsEmpty) {
    LoopbackBus bus;
    TestRunner checks{bus};
    FaultInjector faults;
    const ScenarioRunner runner{bus, checks, faults};
    EXPECT_EQ(runner.subsystem_count(), 0u);
}

TEST(ScenarioRunner, DISABLED_RefusesAClosedBus) {
    LoopbackBus bus;
    TestRunner checks{bus};
    FaultInjector faults;
    ScenarioRunner runner{bus, checks, faults};

    Scenario scenario;
    scenario.name = "closed";
    scenario.bus = "loopback";
    ScenarioResult result;
    result.scenario_name = "untouched";

    const aerotest::Status status = runner.run(scenario, result);
    EXPECT_EQ(status.code, Error::NotOpen);
    EXPECT_EQ(result.scenario_name, "untouched");
    EXPECT_TRUE(result.checks.empty());
}

TEST(ScenarioRunner, DISABLED_RefusesAMismatchedBusName) {
    LoopbackBus bus;
    TestRunner checks{bus};
    FaultInjector faults;
    ScenarioRunner runner{bus, checks, faults};

    Scenario scenario;
    scenario.name = "wrong-bus";
    scenario.bus = "vcan0";
    ScenarioResult result;
    result.scenario_name = "untouched";

    const aerotest::Status status = runner.run(scenario, result);
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(result.scenario_name, "untouched");
}

TEST(ScenarioRunner, DISABLED_WaitRecordsAPass) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    TestRunner checks{bus};
    FaultInjector faults;
    ScenarioRunner runner{bus, checks, faults};

    Scenario scenario;
    scenario.name = "pause";
    scenario.bus = "loopback";
    ScenarioStep step;
    step.kind = StepKind::Wait;
    step.name = "pause";
    step.requirement_id = "REQ-WAIT-001";
    step.wait = std::chrono::milliseconds{0};
    scenario.steps.push_back(step);

    ScenarioResult result;
    ASSERT_TRUE(runner.run(scenario, result).ok());
    EXPECT_EQ(result.scenario_name, "pause");
    ASSERT_EQ(result.checks.size(), 1u);
    EXPECT_TRUE(result.checks[0].passed);
    EXPECT_EQ(result.checks[0].name, "pause");
    EXPECT_EQ(result.checks[0].requirement_id, "REQ-WAIT-001");
    EXPECT_TRUE(result.passed());
}

TEST(ScenarioRunner, DISABLED_AirDataSmokeRecordsTheRequirement) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    AirDataSubsystem air;
    TestRunner checks{bus};
    FaultInjector faults;
    ScenarioRunner runner{bus, checks, faults};
    runner.add_subsystem(air);

    Scenario scenario;
    scenario.name = "air-data-smoke";
    scenario.bus = "loopback";
    scenario.subsystems.push_back("air-data");
    scenario.steps.push_back(send_step(kAirDataRequestId, "REQ-AD-001"));

    ScenarioStep expect;
    expect.kind = StepKind::Expect;
    expect.name = "indicated altitude and airspeed";
    expect.requirement_id = "REQ-AD-001";
    expect.timeout = std::chrono::milliseconds{20};
    expect.message.id = kAirDataReplyId;
    expect.message.data = bytes({100, 0, 50, 0});
    scenario.steps.push_back(expect);

    ScenarioResult result;
    ASSERT_TRUE(runner.run(scenario, result).ok());
    EXPECT_TRUE(result.passed()) << result.failure_count();
    ASSERT_EQ(result.checks.size(), 2u);
    EXPECT_EQ(result.checks[0].requirement_id, "REQ-AD-001");
    EXPECT_EQ(result.checks[1].name, "indicated altitude and airspeed");
    EXPECT_EQ(result.checks[1].requirement_id, "REQ-AD-001");
    EXPECT_TRUE(result.checks[1].passed);
}

TEST(ScenarioRunner, DISABLED_EngineDropPassesWhenTheReplyIsAbsent) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    EngineSubsystem engine;
    TestRunner checks{bus};
    FaultInjector faults;
    ScenarioRunner runner{bus, checks, faults};
    runner.add_subsystem(engine);

    Scenario scenario;
    scenario.name = "engine-drop";
    scenario.bus = "loopback";
    scenario.subsystems.push_back("engine");

    ScenarioStep inject;
    inject.kind = StepKind::Inject;
    inject.name = "drop the next reply";
    inject.requirement_id = "REQ-ENG-001";
    inject.fault.kind = FaultKind::Drop;
    inject.fault.match_id = kEngineReplyId;
    inject.fault.remaining = 1;
    scenario.steps.push_back(inject);
    scenario.steps.push_back(send_step(kEngineRequestId, "REQ-ENG-001"));

    ScenarioStep expect;
    expect.kind = StepKind::Expect;
    expect.name = "reply must not arrive";
    expect.requirement_id = "REQ-ENG-001";
    expect.absent = true;
    expect.timeout = std::chrono::milliseconds{20};
    expect.message.id = kEngineReplyId;
    scenario.steps.push_back(expect);

    ScenarioResult result;
    ASSERT_TRUE(runner.run(scenario, result).ok());
    EXPECT_TRUE(result.passed()) << result.failure_count();
    ASSERT_EQ(result.checks.size(), 3u);
    EXPECT_TRUE(result.checks[2].passed);
    EXPECT_EQ(result.checks[2].requirement_id, "REQ-ENG-001");
}
