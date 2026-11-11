// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/fault_injector.hpp"
#include "aerotest/loopback_bus.hpp"
#include "aerotest/test_runner.hpp"

#include "support.hpp"

#include <chrono>
#include <cstdint>
#include <initializer_list>

using aerotest::CheckResult;
using aerotest::FaultInjector;
using aerotest::FaultKind;
using aerotest::FaultSpec;
using aerotest::LoopbackBus;
using aerotest::Message;
using aerotest::TestRunner;
using aerotest::test::bytes;

namespace {

Message frame(std::uint32_t id, std::initializer_list<unsigned int> data) {
    Message message;
    message.id = id;
    message.data = bytes(data);
    return message;
}

} // namespace

TEST(TestRunner, FreshReportIsAPass) {
    LoopbackBus bus;
    const TestRunner runner{bus};
    EXPECT_TRUE(runner.report().passed());
    EXPECT_EQ(runner.report().failure_count(), 0u);
    EXPECT_TRUE(runner.report().checks.empty());
}

TEST(TestRunner, ReportCountsFailures) {
    TestRunner::Report report;
    EXPECT_TRUE(report.passed());
    EXPECT_EQ(report.failure_count(), 0u);

    report.checks.push_back(CheckResult{"a", true, {}});
    EXPECT_TRUE(report.passed());
    EXPECT_EQ(report.failure_count(), 0u);

    report.checks.push_back(CheckResult{"b", false, "no"});
    EXPECT_FALSE(report.passed());
    EXPECT_EQ(report.failure_count(), 1u);
    EXPECT_EQ(report.checks.size(), 2u);
}

TEST(TestRunner, DISABLED_EchoMatchesOnLoopback) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    TestRunner runner{bus};

    // timestamp on the outbound frame stays 0. The bus stamps the received copy.
    // This check still passes: the runner does not compare timestamp.
    const Message message = frame(0x101, {0x01, 0x02, 0x03});
    const CheckResult result = runner.expect_echo("echo", message, std::chrono::milliseconds{20});
    EXPECT_TRUE(result.passed) << result.detail;
    EXPECT_EQ(result.name, "echo");
    EXPECT_TRUE(runner.report().passed());
    EXPECT_EQ(runner.report().failure_count(), 0u);
    EXPECT_EQ(runner.report().checks.size(), 1u);
    EXPECT_EQ(message.timestamp.count(), 0);
}

TEST(TestRunner, DISABLED_ClosedBusIsARecordedFailure) {
    LoopbackBus bus;
    TestRunner runner{bus};
    const CheckResult result =
        runner.expect_echo("closed", frame(0x1, {0x01}), std::chrono::milliseconds{10});
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.detail.empty());
    ASSERT_EQ(runner.report().checks.size(), 1u);
    EXPECT_EQ(runner.report().checks[0].name, "closed");
    EXPECT_FALSE(runner.report().checks[0].passed);
    EXPECT_FALSE(runner.report().passed());
    EXPECT_EQ(runner.report().failure_count(), 1u);
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(TestRunner, DISABLED_TimeoutIsARecordedFailure) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    TestRunner runner{bus};
    const CheckResult result =
        runner.expect_message("missing", frame(0x5, {0x01}), std::chrono::milliseconds{10});
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.detail.empty());
    ASSERT_EQ(runner.report().checks.size(), 1u);
    EXPECT_EQ(runner.report().checks[0].name, "missing");
    EXPECT_FALSE(runner.report().checks[0].passed);
    EXPECT_EQ(runner.report().failure_count(), 1u);
}

TEST(TestRunner, DISABLED_MismatchIsARecordedFailure) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    TestRunner runner{bus};

    const Message message = frame(0x3, {0x10});
    const CheckResult clean = runner.expect_echo("clean", message, std::chrono::milliseconds{10});
    ASSERT_TRUE(clean.passed) << clean.detail;

    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Corrupt;
    spec.byte_index = 0;
    spec.xor_mask = 0xFF;
    ASSERT_TRUE(injector.arm(spec).ok());
    bus.set_fault_injector(&injector);

    const CheckResult dirty = runner.expect_echo("dirty", message, std::chrono::milliseconds{10});
    EXPECT_FALSE(dirty.passed);
    EXPECT_FALSE(dirty.detail.empty());
    EXPECT_EQ(message.data, bytes({0x10}));
    EXPECT_EQ(runner.report().checks.size(), 2u);
    EXPECT_EQ(runner.report().failure_count(), 1u);
    EXPECT_FALSE(runner.report().passed());
    EXPECT_TRUE(runner.report().checks[0].passed);
    EXPECT_FALSE(runner.report().checks[1].passed);
    EXPECT_EQ(runner.report().checks[1].name, "dirty");
}
