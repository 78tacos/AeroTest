// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace aerotest::test {

/// Build a payload without a narrowing warning on each hex literal.
inline std::vector<std::uint8_t> bytes(std::initializer_list<unsigned int> values) {
    std::vector<std::uint8_t> out;
    out.reserve(values.size());
    for (const unsigned int value : values) {
        out.push_back(static_cast<std::uint8_t>(value));
    }
    return out;
}

} // namespace aerotest::test
