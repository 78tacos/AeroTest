// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/message.hpp"

#include "support.hpp"

#include <chrono>
#include <cstdint>

using aerotest::Error;
using aerotest::IdKind;
using aerotest::Message;
using aerotest::Status;
using aerotest::test::bytes;

TEST(Message, DefaultsAreEmpty) {
    const Message message;
    EXPECT_EQ(message.id, 0u);
    EXPECT_EQ(message.id_kind, IdKind::Standard);
    EXPECT_TRUE(message.data.empty());
    EXPECT_FALSE(message.remote_request);
    EXPECT_EQ(message.timestamp.count(), 0);
}

TEST(Message, AssigningFieldsRoundTrips) {
    Message message;
    message.id = 0x42;
    message.id_kind = IdKind::Extended;
    message.data = bytes({0x01, 0x02, 0x03});
    message.remote_request = true;
    message.timestamp = std::chrono::nanoseconds{10};

    EXPECT_EQ(message.id, 0x42u);
    EXPECT_EQ(message.id_kind, IdKind::Extended);
    EXPECT_EQ(message.data, bytes({0x01, 0x02, 0x03}));
    EXPECT_TRUE(message.remote_request);
    EXPECT_EQ(message.timestamp.count(), 10);
}

TEST(Message, DISABLED_AcceptsMaxStandardFrame) {
    Message message;
    message.id = 0x7FF;
    message.data = bytes({1, 2, 3, 4, 5, 6, 7, 8});
    const Status status = message.validate();
    EXPECT_TRUE(status.ok()) << status.detail;
}

TEST(Message, DISABLED_AcceptsExtendedPast11Bits) {
    Message message;
    message.id = 0x800;
    message.id_kind = IdKind::Extended;
    const Status status = message.validate();
    EXPECT_TRUE(status.ok()) << status.detail;
}

TEST(Message, DISABLED_AcceptsMaxExtendedId) {
    Message message;
    message.id = 0x1FFFFFFF;
    message.id_kind = IdKind::Extended;
    message.data = bytes({0xAA});
    const Status status = message.validate();
    EXPECT_TRUE(status.ok()) << status.detail;
}

TEST(Message, DISABLED_AcceptsRemoteRequestWithoutData) {
    Message message;
    message.id = 0x100;
    message.remote_request = true;
    const Status status = message.validate();
    EXPECT_TRUE(status.ok()) << status.detail;
}

TEST(Message, DISABLED_RejectsStandardIdPast11Bits) {
    Message message;
    message.id = 0x800;
    const Status status = message.validate();
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, Error::InvalidArgument);
}

TEST(Message, DISABLED_RejectsExtendedIdPast29Bits) {
    Message message;
    message.id = 0x20000000;
    message.id_kind = IdKind::Extended;
    const Status status = message.validate();
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, Error::InvalidArgument);
}

TEST(Message, DISABLED_RejectsNinthDataByte) {
    Message message;
    message.id = 0x10;
    message.data = bytes({1, 2, 3, 4, 5, 6, 7, 8, 9});
    const Status status = message.validate();
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, Error::InvalidArgument);
}

TEST(Message, DISABLED_RejectsRemoteRequestWithData) {
    Message message;
    message.id = 0x10;
    message.remote_request = true;
    message.data = bytes({0x01});
    const Status status = message.validate();
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, Error::InvalidArgument);
}
