// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/status.hpp"

#include <string>

using aerotest::Error;
using aerotest::Status;

TEST(Status, OkIsTheOnlySuccess) {
    const Status ok = Status::success();
    EXPECT_TRUE(ok.ok());
    EXPECT_EQ(ok.code, Error::Ok);
    EXPECT_TRUE(ok.detail.empty());

    const Status missing = Status::not_implemented("demo");
    EXPECT_FALSE(missing.ok());
    EXPECT_EQ(missing.code, Error::NotImplemented);
    EXPECT_EQ(missing.detail, "demo");

    const Status closed = Status::failure(Error::NotOpen, "closed");
    EXPECT_FALSE(closed.ok());
    EXPECT_EQ(closed.code, Error::NotOpen);
    EXPECT_EQ(closed.detail, "closed");
}

TEST(Status, DISABLED_ToStringSpellsEveryError) {
    EXPECT_STREQ(aerotest::to_string(Error::Ok), "ok");
    EXPECT_STREQ(aerotest::to_string(Error::NotOpen), "not_open");
    EXPECT_STREQ(aerotest::to_string(Error::Timeout), "timeout");
    EXPECT_STREQ(aerotest::to_string(Error::InvalidArgument), "invalid_argument");
    EXPECT_STREQ(aerotest::to_string(Error::Io), "io");
    EXPECT_STREQ(aerotest::to_string(Error::FaultInjected), "fault_injected");
    EXPECT_STREQ(aerotest::to_string(Error::NotImplemented), "not_implemented");
}
