// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/fault_injector.hpp"
#include "aerotest/loopback_bus.hpp"

#include "support.hpp"

#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <thread>

using aerotest::Error;
using aerotest::FaultInjector;
using aerotest::FaultKind;
using aerotest::FaultSpec;
using aerotest::IdKind;
using aerotest::LoopbackBus;
using aerotest::Message;
using aerotest::Status;
using aerotest::test::bytes;

namespace {

Message frame(std::uint32_t id, std::initializer_list<unsigned int> data) {
    Message message;
    message.id = id;
    message.data = bytes(data);
    return message;
}

} // namespace

TEST(LoopbackBus, NameIsLoopback) {
    const LoopbackBus bus;
    EXPECT_EQ(bus.name(), "loopback");
}

TEST(LoopbackBus, StartsClosed) {
    const LoopbackBus bus;
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(LoopbackBus, FaultSlotStartsEmpty) {
    LoopbackBus bus;
    EXPECT_EQ(bus.fault_injector(), nullptr);
    FaultInjector injector;
    bus.set_fault_injector(&injector);
    EXPECT_EQ(bus.fault_injector(), &injector);
}

TEST(LoopbackBus, DISABLED_OpenThenIsOpen) {
    LoopbackBus bus;
    const Status status = bus.open();
    ASSERT_TRUE(status.ok()) << status.detail;
    EXPECT_TRUE(bus.is_open());
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(LoopbackBus, DISABLED_SecondOpenKeepsQueuedFrames) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    const Message message = frame(0x11, {0x01});
    ASSERT_TRUE(bus.send(message).ok());

    const Status again = bus.open();
    ASSERT_TRUE(again.ok()) << again.detail;
    EXPECT_TRUE(bus.is_open());
    EXPECT_EQ(bus.queued(), 1u);
}

TEST(LoopbackBus, DISABLED_CloseWhenClosedIsOk) {
    LoopbackBus bus;
    const Status status = bus.close();
    EXPECT_TRUE(status.ok()) << status.detail;
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(LoopbackBus, DISABLED_CloseDropsQueuedFrames) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    ASSERT_TRUE(bus.send(frame(0x11, {0x01})).ok());
    const Status status = bus.close();
    ASSERT_TRUE(status.ok()) << status.detail;
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.queued(), 0u);

    const Status after = bus.send(frame(0x11, {0x01}));
    EXPECT_EQ(after.code, Error::NotOpen);
}

TEST(LoopbackBus, DISABLED_SendBeforeOpen) {
    LoopbackBus bus;
    const Status status = bus.send(frame(0x1, {0x02}));
    EXPECT_EQ(status.code, Error::NotOpen);
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(LoopbackBus, DISABLED_ReceiveBeforeOpen) {
    LoopbackBus bus;
    Message out = frame(0x55, {0x09});
    const Status status = bus.receive(out, std::chrono::milliseconds{0});
    EXPECT_EQ(status.code, Error::NotOpen);
    EXPECT_EQ(out.id, 0x55u);
    EXPECT_EQ(out.data, bytes({0x09}));
}

TEST(LoopbackBus, DISABLED_NegativeTimeoutIsInvalid) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    ASSERT_TRUE(bus.send(frame(0x1, {0x01})).ok());

    Message out;
    const Status status = bus.receive(out, std::chrono::milliseconds{-1});
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(bus.queued(), 1u);
    EXPECT_EQ(out.id, 0u);

    Message taken;
    const Status ok = bus.receive(taken, std::chrono::milliseconds{0});
    ASSERT_TRUE(ok.ok()) << ok.detail;
    EXPECT_EQ(taken.id, 0x1u);
}

TEST(LoopbackBus, DISABLED_RejectsInvalidFrame) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    Message message = frame(0x1, {1, 2, 3, 4, 5, 6, 7, 8, 9});
    const Status status = bus.send(message);
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(bus.queued(), 0u);
    EXPECT_EQ(message.data.size(), 9u);
}

TEST(LoopbackBus, DISABLED_RoundTrip) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());

    Message message;
    message.id = 0x123;
    message.id_kind = IdKind::Extended;
    message.data = bytes({0xDE, 0xAD});
    message.remote_request = false;
    const Status sent = bus.send(message);
    ASSERT_TRUE(sent.ok()) << sent.detail;
    EXPECT_EQ(message.data, bytes({0xDE, 0xAD}));
    EXPECT_EQ(bus.queued(), 1u);

    Message received;
    const Status status = bus.receive(received, std::chrono::milliseconds{0});
    ASSERT_TRUE(status.ok()) << status.detail;
    EXPECT_EQ(received.id, message.id);
    EXPECT_EQ(received.id_kind, IdKind::Extended);
    EXPECT_EQ(received.data, message.data);
    EXPECT_FALSE(received.remote_request);
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(LoopbackBus, DISABLED_FifoOrder) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    ASSERT_TRUE(bus.send(frame(0x1, {0x01})).ok());
    ASSERT_TRUE(bus.send(frame(0x2, {0x02})).ok());

    Message first;
    Message second;
    ASSERT_TRUE(bus.receive(first, std::chrono::milliseconds{0}).ok());
    ASSERT_TRUE(bus.receive(second, std::chrono::milliseconds{0}).ok());
    EXPECT_EQ(first.id, 0x1u);
    EXPECT_EQ(second.id, 0x2u);
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(LoopbackBus, DISABLED_ReceiveOnEmptyTimesOut) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    Message out = frame(0x7, {0x03});
    const Status status = bus.receive(out, std::chrono::milliseconds{50});
    EXPECT_EQ(status.code, Error::Timeout);
    EXPECT_EQ(out.id, 0x7u);
    EXPECT_EQ(out.data, bytes({0x03}));
}

TEST(LoopbackBus, DISABLED_TimeoutLeavesOutputUntouched) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    Message out;
    out.id = 0x55;
    out.data = bytes({0x09});
    const Status status = bus.receive(out, std::chrono::milliseconds{0});
    EXPECT_EQ(status.code, Error::Timeout);
    EXPECT_EQ(out.id, 0x55u);
    EXPECT_EQ(out.data, bytes({0x09}));
}

TEST(LoopbackBus, DISABLED_StampsTimestamp) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    const auto before = std::chrono::steady_clock::now().time_since_epoch();
    ASSERT_TRUE(bus.send(frame(0x1, {0x01})).ok());

    Message received;
    ASSERT_TRUE(bus.receive(received, std::chrono::milliseconds{0}).ok());
    EXPECT_GE(received.timestamp, before);
}

TEST(LoopbackBus, DISABLED_DropFaultIsNotDelivered) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Drop;
    ASSERT_TRUE(injector.arm(spec).ok());
    bus.set_fault_injector(&injector);

    const Status sent = bus.send(frame(0x8, {0x01}));
    EXPECT_TRUE(sent.ok()) << sent.detail;
    EXPECT_EQ(bus.queued(), 0u);

    Message out;
    EXPECT_EQ(bus.receive(out, std::chrono::milliseconds{0}).code, Error::Timeout);
}

TEST(LoopbackBus, DISABLED_CorruptFaultChangesOnlyTheCopy) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Corrupt;
    spec.byte_index = 0;
    spec.xor_mask = 0xFF;
    ASSERT_TRUE(injector.arm(spec).ok());
    bus.set_fault_injector(&injector);

    Message original = frame(0x7, {0x01});
    const Status sent = bus.send(original);
    ASSERT_TRUE(sent.ok()) << sent.detail;
    EXPECT_EQ(original.data, bytes({0x01}));

    Message received;
    ASSERT_TRUE(bus.receive(received, std::chrono::milliseconds{0}).ok());
    EXPECT_EQ(received.id, 0x7u);
    EXPECT_EQ(received.data, bytes({0xFE}));
}

TEST(LoopbackBus, DISABLED_ErrorFaultFailsTheSend) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Error;
    ASSERT_TRUE(injector.arm(spec).ok());
    bus.set_fault_injector(&injector);

    const Status sent = bus.send(frame(0x7, {0x01}));
    EXPECT_EQ(sent.code, Error::FaultInjected);
    EXPECT_EQ(bus.queued(), 0u);
}

TEST(LoopbackBus, DISABLED_DelayFaultWaitsForTheClock) {
    LoopbackBus bus;
    ASSERT_TRUE(bus.open().ok());
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Delay;
    spec.delay = std::chrono::milliseconds{50};
    ASSERT_TRUE(injector.arm(spec).ok());
    bus.set_fault_injector(&injector);

    ASSERT_TRUE(bus.send(frame(0x15, {0x0A})).ok());
    EXPECT_EQ(bus.queued(), 1u);

    Message early;
    const Status too_soon = bus.receive(early, std::chrono::milliseconds{0});
    EXPECT_EQ(too_soon.code, Error::Timeout);
    EXPECT_EQ(bus.queued(), 1u);
    EXPECT_EQ(early.id, 0u);

    std::this_thread::sleep_for(std::chrono::milliseconds{80});

    Message received;
    const Status status = bus.receive(received, std::chrono::milliseconds{0});
    ASSERT_TRUE(status.ok()) << status.detail;
    EXPECT_EQ(received.id, 0x15u);
    EXPECT_EQ(received.data, bytes({0x0A}));
    EXPECT_EQ(bus.queued(), 0u);
}
