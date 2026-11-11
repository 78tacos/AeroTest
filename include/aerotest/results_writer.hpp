// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include "aerotest/status.hpp"
#include "aerotest/test_runner.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace aerotest {

/// What one scenario run produced. The writers print this. They do not rerun it.
///
/// `passed()` and `failure_count()` are done, same idea as `TestRunner::Report`.
struct ScenarioResult {
    std::string scenario_name;
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

/// Prints a `ScenarioResult`. Task T2 in docs/step-2.md. Both writers are stubs.
///
/// JSON is one line plus a trailing newline. Keys, in this order, no extra spaces:
///
/// ```
/// {"name":"...","passed":true,"checks":[{"name":"...","passed":true,"requirement":"...","detail":"..."}]}
/// ```
///
/// `passed` is the JSON boolean from `ScenarioResult::passed()`, not a string.
/// Strings escape `\`, `"`, and newline (`\n`). No other escaping is required.
///
/// JUnit is the CI-oriented twin. Two-space indent, a trailing newline, and this shape:
///
/// ```
/// <?xml version="1.0" encoding="UTF-8"?>
/// <testsuite name="NAME" tests="N" failures="F">
///   <testcase name="CHECK" classname="REQ"/>
///   <testcase name="CHECK" classname="REQ">
///     <failure message="failed">DETAIL</failure>
///   </testcase>
/// </testsuite>
/// ```
///
/// A passing check is one self-closing `testcase`. A failing check has a
/// `failure` child whose body is `detail`. `classname` is the requirement id
/// (empty when the check has none). `tests` is the check count. `failures` is
/// `failure_count()`. Escape `&`, `<`, `>`, and `"` in `name`, `classname`,
/// and `detail` as `&amp;`, `&lt;`, `&gt;`, and `&quot;`.
class ResultsWriter {
  public:
    virtual ~ResultsWriter() = default;

    ResultsWriter(const ResultsWriter&) = delete;
    ResultsWriter& operator=(const ResultsWriter&) = delete;

    virtual Status write(const ScenarioResult& result, std::ostream& out) const = 0;

  protected:
    ResultsWriter() = default;
};

class JsonResultsWriter final : public ResultsWriter {
  public:
    Status write(const ScenarioResult& result, std::ostream& out) const override;
};

class JunitResultsWriter final : public ResultsWriter {
  public:
    Status write(const ScenarioResult& result, std::ostream& out) const override;
};

} // namespace aerotest
