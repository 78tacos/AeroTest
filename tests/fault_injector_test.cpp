// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/fault_injector.hpp"

#include "support.hpp"

#include <chrono>
#include <cstdint>
#include <initializer_list>

using aerotest::Error;
using aerotest::FaultEffect;
using aerotest::FaultInjector;
using aerotest::FaultKind;
using aerotest::FaultSpec;
using aerotest::Message;
using aerotest::Status;
using aerotest::test::bytes;

namespace {

Message payload(std::uint32_t id, std::initializer_list<unsigned int> data) {
    Message message;
    message.id = id;
    message.data = bytes(data);
    return message;
}

} // namespace

TEST(FaultInjector, StartsEmpty) {
    const FaultInjector injector;
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, ClearOnEmptyIsSafe) {
    FaultInjector injector;
    injector.clear();
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_ArmIncreasesPending) {
    FaultInjector injector;
    const Status first = injector.arm(FaultSpec{});
    ASSERT_TRUE(first.ok()) << first.detail;
    EXPECT_EQ(injector.pending(), 1u);

    const Status second = injector.arm(FaultSpec{});
    ASSERT_TRUE(second.ok()) << second.detail;
    EXPECT_EQ(injector.pending(), 2u);
}

TEST(FaultInjector, DISABLED_RejectsZeroRemaining) {
    FaultInjector injector;
    FaultSpec spec;
    spec.remaining = 0;
    const Status status = injector.arm(spec);
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_RejectsRemainingBelowMinusOne) {
    FaultInjector injector;
    FaultSpec spec;
    spec.remaining = -2;
    const Status status = injector.arm(spec);
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_EmptyInjectorPassesThrough) {
    FaultInjector injector;
    Message message = payload(0x1, {0x11});
    const FaultEffect effect = injector.apply_tx(message);
    EXPECT_TRUE(effect.ok()) << static_cast<int>(effect.code);
    EXPECT_TRUE(effect.deliver);
    EXPECT_EQ(effect.extra_delay.count(), 0);
    EXPECT_EQ(message.data, bytes({0x11}));
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_DropSuppressesDelivery) {
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Drop;
    ASSERT_TRUE(injector.arm(spec).ok());

    Message message = payload(0x5, {0x01, 0x02});
    const FaultEffect effect = injector.apply_tx(message);
    EXPECT_TRUE(effect.ok());
    EXPECT_FALSE(effect.deliver);
    EXPECT_EQ(message.data, bytes({0x01, 0x02}));
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_CorruptFlipsTheByte) {
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Corrupt;
    spec.byte_index = 1;
    spec.xor_mask = 0x0F;
    ASSERT_TRUE(injector.arm(spec).ok());

    Message message = payload(0x42, {0x10, 0x20});
    const FaultEffect effect = injector.apply_tx(message);
    EXPECT_TRUE(effect.ok());
    EXPECT_TRUE(effect.deliver);
    EXPECT_EQ(effect.extra_delay.count(), 0);
    EXPECT_EQ(message.data, bytes({0x10, 0x2F}));
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_CorruptRejectsABadIndex) {
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Corrupt;
    spec.byte_index = 2;
    ASSERT_TRUE(injector.arm(spec).ok());

    Message message = payload(0x1, {0xAA, 0xBB});
    const FaultEffect effect = injector.apply_tx(message);
    EXPECT_EQ(effect.code, Error::InvalidArgument);
    EXPECT_FALSE(effect.deliver);
    EXPECT_EQ(message.data, bytes({0xAA, 0xBB}));
    // A spec that cannot be applied is still armed.
    EXPECT_EQ(injector.pending(), 1u);
}

TEST(FaultInjector, DISABLED_ErrorReportsFaultInjected) {
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Error;
    ASSERT_TRUE(injector.arm(spec).ok());

    Message message = payload(0x9, {0x01});
    const FaultEffect effect = injector.apply_tx(message);
    EXPECT_EQ(effect.code, Error::FaultInjected);
    EXPECT_FALSE(effect.deliver);
    EXPECT_EQ(message.data, bytes({0x01}));
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_DelaySetsExtraDelay) {
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Delay;
    spec.delay = std::chrono::milliseconds{25};
    ASSERT_TRUE(injector.arm(spec).ok());

    Message message = payload(0x3, {0x04});
    const FaultEffect effect = injector.apply_tx(message);
    EXPECT_TRUE(effect.ok());
    EXPECT_TRUE(effect.deliver);
    EXPECT_EQ(effect.extra_delay, std::chrono::milliseconds{25});
    EXPECT_EQ(message.data, bytes({0x04}));
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_OtherIdsDoNotMatch) {
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Corrupt;
    spec.match_id = 0x10;
    spec.byte_index = 0;
    spec.xor_mask = 0xFF;
    ASSERT_TRUE(injector.arm(spec).ok());

    Message message = payload(0x11, {0x01});
    const FaultEffect effect = injector.apply_tx(message);
    EXPECT_TRUE(effect.ok());
    EXPECT_TRUE(effect.deliver);
    EXPECT_EQ(message.data, bytes({0x01}));
    EXPECT_EQ(injector.pending(), 1u);
}

TEST(FaultInjector, DISABLED_RemainingCountsDown) {
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Drop;
    spec.remaining = 2;
    ASSERT_TRUE(injector.arm(spec).ok());

    Message first = payload(0x1, {0x01});
    const FaultEffect once = injector.apply_tx(first);
    EXPECT_FALSE(once.deliver);
    EXPECT_EQ(injector.pending(), 1u);

    Message second = payload(0x1, {0x01});
    const FaultEffect twice = injector.apply_tx(second);
    EXPECT_FALSE(twice.deliver);
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_NegativeOneDoesNotExpire) {
    FaultInjector injector;
    FaultSpec spec;
    spec.kind = FaultKind::Drop;
    spec.remaining = -1;
    ASSERT_TRUE(injector.arm(spec).ok());

    Message first = payload(0x1, {0x01});
    EXPECT_FALSE(injector.apply_tx(first).deliver);
    Message second = payload(0x1, {0x02});
    EXPECT_FALSE(injector.apply_tx(second).deliver);
    EXPECT_EQ(injector.pending(), 1u);
}

TEST(FaultInjector, DISABLED_FirstArmedFaultWins) {
    FaultInjector injector;

    FaultSpec drop;
    drop.kind = FaultKind::Drop;
    ASSERT_TRUE(injector.arm(drop).ok());

    FaultSpec corrupt;
    corrupt.kind = FaultKind::Corrupt;
    corrupt.byte_index = 0;
    corrupt.xor_mask = 0xFF;
    ASSERT_TRUE(injector.arm(corrupt).ok());

    Message first = payload(0x4, {0x01});
    const FaultEffect dropped = injector.apply_tx(first);
    EXPECT_TRUE(dropped.ok());
    EXPECT_FALSE(dropped.deliver);
    EXPECT_EQ(first.data, bytes({0x01}));
    EXPECT_EQ(injector.pending(), 1u);

    Message second = payload(0x4, {0x01});
    const FaultEffect flipped = injector.apply_tx(second);
    EXPECT_TRUE(flipped.ok());
    EXPECT_TRUE(flipped.deliver);
    EXPECT_EQ(second.data, bytes({0xFE}));
    EXPECT_EQ(injector.pending(), 0u);
}

TEST(FaultInjector, DISABLED_ClearForgetsArmedFaults) {
    FaultInjector injector;
    ASSERT_TRUE(injector.arm(FaultSpec{}).ok());
    ASSERT_TRUE(injector.arm(FaultSpec{}).ok());
    injector.clear();
    EXPECT_EQ(injector.pending(), 0u);

    Message message = payload(0x1, {0x22});
    const FaultEffect effect = injector.apply_tx(message);
    EXPECT_TRUE(effect.ok());
    EXPECT_TRUE(effect.deliver);
    EXPECT_EQ(message.data, bytes({0x22}));
}
