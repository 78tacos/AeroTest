// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/status.hpp"

namespace aerotest {

const char* to_string(Error error) {
    // TODO(step-1): task T1 in docs/step-1.md.
    // Return the spelling listed there for every enumerator.
    // Do not add a `default` label. A default hides a new enumerator from -Wswitch.
    (void)error;
    return "unknown";
}

} // namespace aerotest
