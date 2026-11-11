// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/version.hpp"

#include <cstring>

// Scaffold checks are ordinary TEST()s and must keep passing.
// Behavior that is still a stub is named DISABLED_ and is listed in docs/step-1.md.

TEST(Version, MatchesCMakeProjectVersion) {
    EXPECT_EQ(aerotest::kVersionMajor, 0);
    EXPECT_EQ(aerotest::kVersionMinor, 1);
    EXPECT_EQ(aerotest::kVersionPatch, 0);
    EXPECT_STREQ(aerotest::kVersion, "0.1.0");
    EXPECT_EQ(std::strlen(aerotest::kVersion), 5u);
}
