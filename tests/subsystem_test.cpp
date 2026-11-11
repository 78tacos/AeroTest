// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/air_data_subsystem.hpp"
#include "aerotest/engine_subsystem.hpp"
#include "aerotest/loopback_bus.hpp"

#include "support.hpp"

#include <chrono>
#include <cstdint>

using aerotest::AirDataSubsystem;
using aerotest::EngineSubsystem;
using aerotest::Error;
using aerotest::kAirDataReplyId;
using aerotest::kAirDataRequestId;
using aerotest::kEngineReplyId;
using aerotest::kEngineRequestId;
using aerotest::LoopbackBus;
using aerotest::Message;
using aerotest::test::bytes;

TEST(AirDataSubsystem, NameAndDefaults) {
    const AirDataSubsystem node;
    EXPECT_EQ(node.name(), "air-data");
    EXPECT_EQ(node.altitude_ft, 100);
    EXPECT_EQ(node.airspeed_kt, 50);
    EXPECT_EQ(kAirDataRequestId, 0x100u);
    EXPECT_EQ(kAirDataReplyId, 0x101u);
}

TEST(EngineSubsystem, NameAndDefaults) {
    const EngineSubsystem node;
    EXPECT_EQ(node.name(), "engine");
    EXPECT_EQ(node.rpm, 2400);
    EXPECT_EQ(node.coolant_c, 90);
    EXPECT_EQ(kEngineRequestId, 0x200u);
    EXPECT_EQ(kEngineReplyId, 0x201u);
}

TEST(AirDataSubsystem, DISABLED_RepliesLittleEndian) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    AirDataSubsystem node;
    Message request;
    request.id = kAirDataRequestId;

    ASSERT_TRUE(node.on_frame(bus, request).ok());
    EXPECT_TRUE(request.data.empty());
    EXPECT_EQ(bus.queued(), 1u);

    Message got;
    ASSERT_TRUE(bus.receive(got, std::chrono::milliseconds{20}).ok());
    EXPECT_EQ(got.id, kAirDataReplyId);
    EXPECT_EQ(got.data, bytes({100, 0, 50, 0}));
}

TEST(AirDataSubsystem, DISABLED_IgnoresOtherIds) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    AirDataSubsystem node;
    Message other;
    other.id = 0x42;
    other.data = bytes({1});

    ASSERT_TRUE(node.on_frame(bus, other).ok());
    EXPECT_EQ(other.data, bytes({1}));
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(AirDataSubsystem, DISABLED_ClosedBusIsNotOpen) {
    LoopbackBus bus;
    AirDataSubsystem node;
    Message request;
    request.id = kAirDataRequestId;

    const aerotest::Status status = node.on_frame(bus, request);
    EXPECT_EQ(status.code, Error::NotOpen);
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(EngineSubsystem, DISABLED_RepliesLittleEndian) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    EngineSubsystem node;
    Message request;
    request.id = kEngineRequestId;

    ASSERT_TRUE(node.on_frame(bus, request).ok());
    EXPECT_EQ(bus.queued(), 1u);

    Message got;
    ASSERT_TRUE(bus.receive(got, std::chrono::milliseconds{20}).ok());
    EXPECT_EQ(got.id, kEngineReplyId);
    // 2400 is bytes 96, 9. 90 C is bytes 90, 0.
    EXPECT_EQ(got.data, bytes({96, 9, 90, 0}));
}

TEST(EngineSubsystem, DISABLED_IgnoresOtherIds) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    EngineSubsystem node;
    Message other;
    other.id = kAirDataRequestId;

    ASSERT_TRUE(node.on_frame(bus, other).ok());
    EXPECT_EQ(bus.queued(), 0u);
}
