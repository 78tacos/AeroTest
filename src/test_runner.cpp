// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/test_runner.hpp"

#include <utility>

namespace aerotest {

TestRunner::TestRunner(Bus& bus) : bus_(bus) {}

CheckResult TestRunner::expect_message(std::string name, const Message& expected,
                                       std::chrono::milliseconds timeout) {
    // TODO(step-1): task T6 in docs/step-1.md.
    // Receive from bus_, compare id / id_kind / remote_request / data, and push
    // the CheckResult onto report_.checks before returning it. Skip timestamp.
    (void)expected;
    (void)timeout;
    return CheckResult{std::move(name), false, std::string(bus_.name()) + ": not implemented"};
}

CheckResult TestRunner::expect_echo(std::string name, const Message& outbound,
                                    std::chrono::milliseconds timeout) {
    // TODO(step-1): task T6 in docs/step-1.md.
    // If the bus is closed, record a failure and do not send.
    // Otherwise send a copy of `outbound`, then compare the way expect_message does.
    (void)outbound;
    (void)timeout;
    return CheckResult{std::move(name), false, std::string(bus_.name()) + ": not implemented"};
}

} // namespace aerotest
