// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

// Kernel-backed tests. They compile everywhere and stay DISABLED_ until task T7.
// ctest presets exclude the needs-vcan label, so CI does not run them.
//
// Before turning the prefix off:
//   sudo modprobe vcan
//   sudo ip link add dev vcan0 type vcan
//   sudo ip link set up vcan0
//   ctest --test-dir build/gcc-debug -L needs-vcan --output-on-failure
//   ./build/gcc-debug/tests/aerotest_vcan_tests --gtest_also_run_disabled_tests

#include <gtest/gtest.h>

#include "aerotest/socketcan_bus.hpp"

#include "support.hpp"

#include <chrono>

using aerotest::Error;
using aerotest::IdKind;
using aerotest::Message;
using aerotest::SocketCanBus;
using aerotest::Status;
using aerotest::test::bytes;

namespace {

constexpr auto kWait = std::chrono::milliseconds{200};

} // namespace

TEST(SocketCanVcan, DISABLED_VcanRoundTrip) {
    SocketCanBus bus{"vcan0"};
    const Status opened = bus.open();
    ASSERT_TRUE(opened.ok()) << opened.detail;
    EXPECT_TRUE(bus.is_open());

    Message message;
    message.id = 0x321;
    message.id_kind = IdKind::Standard;
    message.data = bytes({0xAA, 0xBB});
    const Status sent = bus.send(message);
    ASSERT_TRUE(sent.ok()) << sent.detail;

    Message received;
    const Status status = bus.receive(received, kWait);
    ASSERT_TRUE(status.ok()) << status.detail;
    EXPECT_EQ(received.id, 0x321u);
    EXPECT_EQ(received.id_kind, IdKind::Standard);
    EXPECT_EQ(received.data, bytes({0xAA, 0xBB}));
    EXPECT_FALSE(received.remote_request);
    EXPECT_GT(received.timestamp.count(), 0);

    Message extra;
    EXPECT_EQ(bus.receive(extra, std::chrono::milliseconds{20}).code, Error::Timeout);

    ASSERT_TRUE(bus.close().ok());
    EXPECT_FALSE(bus.is_open());
    const Status reopened = bus.open();
    ASSERT_TRUE(reopened.ok()) << reopened.detail;
    EXPECT_TRUE(bus.is_open());
}

TEST(SocketCanVcan, DISABLED_VcanExtendedRoundTrip) {
    SocketCanBus bus{"vcan0"};
    ASSERT_TRUE(bus.open().ok());

    Message message;
    message.id = 0x18FF50E5;
    message.id_kind = IdKind::Extended;
    message.data = bytes({0x01, 0x02, 0x03});
    ASSERT_TRUE(bus.send(message).ok());

    Message received;
    ASSERT_TRUE(bus.receive(received, kWait).ok());
    EXPECT_EQ(received.id, 0x18FF50E5u);
    EXPECT_EQ(received.id_kind, IdKind::Extended);
    EXPECT_EQ(received.data, message.data);
}

TEST(SocketCanVcan, DISABLED_VcanRemoteFrame) {
    SocketCanBus bus{"vcan0"};
    ASSERT_TRUE(bus.open().ok());

    Message message;
    message.id = 0x42;
    message.remote_request = true;
    ASSERT_TRUE(bus.send(message).ok());

    Message received;
    ASSERT_TRUE(bus.receive(received, kWait).ok());
    EXPECT_EQ(received.id, 0x42u);
    EXPECT_TRUE(received.remote_request);
    EXPECT_TRUE(received.data.empty());
}
