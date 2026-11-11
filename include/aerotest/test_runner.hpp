// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/bus.hpp"

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace aerotest {

/// One named check. `detail` is empty when the check passed.
///
/// `requirement_id` is empty unless a scenario step filled it (step 2).
/// Step 1's runner leaves it empty. It is the last field, with a default
/// member initializer, so `CheckResult{"name", true, {}}` still compiles
/// and does not trip `-Wmissing-field-initializers`.
struct CheckResult {
    std::string name;
    bool passed{false};
    std::string detail;
    std::string requirement_id{};
};

/// Sends a frame and checks what comes back. The bus must already be open.
/// The runner does not call `open` or `close`.
///
/// `report()` bookkeeping helpers are done. `expect_message` and `expect_echo`
/// are task T6. They must append every check to the report before returning it,
/// including failures.
class TestRunner {
  public:
    explicit TestRunner(Bus& bus);

    TestRunner(const TestRunner&) = delete;
    TestRunner& operator=(const TestRunner&) = delete;

    /// Receive one frame and compare `id`, `id_kind`, `remote_request`, and `data`.
    /// Do not compare `timestamp`.
    CheckResult expect_message(std::string name, const Message& expected,
                               std::chrono::milliseconds timeout);

    /// `send` a copy of `outbound`, then compare the received frame the same way
    /// `expect_message` does. The caller's `outbound` stays unchanged.
    CheckResult expect_echo(std::string name, const Message& outbound,
                            std::chrono::milliseconds timeout);

    struct Report {
        std::vector<CheckResult> checks;

        [[nodiscard]] bool passed() const {
            for (const CheckResult& check : checks) {
                if (!check.passed) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] std::size_t failure_count() const {
            std::size_t count = 0;
            for (const CheckResult& check : checks) {
                if (!check.passed) {
                    ++count;
                }
            }
            return count;
        }
    };

    [[nodiscard]] const Report& report() const noexcept { return report_; }

  private:
    Bus& bus_;
    Report report_;
};

} // namespace aerotest
