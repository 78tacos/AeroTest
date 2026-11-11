// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/results_writer.hpp"

#include <sstream>
#include <string>

using aerotest::CheckResult;
using aerotest::JsonResultsWriter;
using aerotest::JunitResultsWriter;
using aerotest::ScenarioResult;

TEST(ScenarioResult, CountsLikeTheReport) {
    ScenarioResult result;
    EXPECT_TRUE(result.passed());
    EXPECT_EQ(result.failure_count(), 0u);

    result.checks.push_back(CheckResult{"a", true, {}, {}});
    EXPECT_TRUE(result.passed());

    result.checks.push_back(CheckResult{"b", false, "no", "REQ-1"});
    EXPECT_FALSE(result.passed());
    EXPECT_EQ(result.failure_count(), 1u);
    EXPECT_EQ(result.checks[1].requirement_id, "REQ-1");
}

TEST(JsonResultsWriter, DISABLED_PassIsOneObject) {
    ScenarioResult result;
    result.scenario_name = "air-data-smoke";
    result.checks.push_back(CheckResult{"indicated", true, {}, "REQ-AD-001"});

    std::ostringstream out;
    ASSERT_TRUE(JsonResultsWriter{}.write(result, out).ok());
    EXPECT_EQ(out.str(),
              "{\"name\":\"air-data-smoke\",\"passed\":true,\"checks\":[{\"name\":\"indicated\","
              "\"passed\":true,\"requirement\":\"REQ-AD-001\",\"detail\":\"\"}]}\n");
}

TEST(JsonResultsWriter, DISABLED_EscapesQuoteBackslashAndNewline) {
    ScenarioResult result;
    result.scenario_name = "q";
    // Decoded detail is: say "no"\<newline>
    result.checks.push_back(CheckResult{"c", false, "say \"no\"\\\n", {}});

    std::ostringstream out;
    ASSERT_TRUE(JsonResultsWriter{}.write(result, out).ok());
    EXPECT_EQ(
        out.str(),
        R"({"name":"q","passed":false,"checks":[{"name":"c","passed":false,"requirement":"","detail":"say \"no\"\\\n"}]})"
        "\n");
}

TEST(JunitResultsWriter, DISABLED_PassAndFailure) {
    ScenarioResult result;
    result.scenario_name = "suite";
    result.checks.push_back(CheckResult{"ok", true, {}, "REQ-1"});
    result.checks.push_back(CheckResult{"bad", false, "a<b>&c", "REQ-2"});

    constexpr char kXml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                            "<testsuite name=\"suite\" tests=\"2\" failures=\"1\">\n"
                            "  <testcase name=\"ok\" classname=\"REQ-1\"/>\n"
                            "  <testcase name=\"bad\" classname=\"REQ-2\">\n"
                            "    <failure message=\"failed\">a&lt;b&gt;&amp;c</failure>\n"
                            "  </testcase>\n"
                            "</testsuite>\n";

    std::ostringstream out;
    ASSERT_TRUE(JunitResultsWriter{}.write(result, out).ok());
    EXPECT_EQ(out.str(), kXml);
}
