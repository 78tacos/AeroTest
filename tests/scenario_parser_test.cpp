// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include <gtest/gtest.h>

#include "aerotest/scenario_parser.hpp"

#include "support.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

using aerotest::test::bytes;

using aerotest::Error;
using aerotest::FaultKind;
using aerotest::Scenario;
using aerotest::StepKind;
using aerotest::YamlScenarioParser;

namespace {

std::filesystem::path scenario_path(const char* name) {
    return std::filesystem::path{AEROTEST_SOURCE_DIR} / "scenarios" / name;
}

} // namespace

TEST(YamlScenarioParser, BackendIsYamlCpp) {
    const YamlScenarioParser parser;
    EXPECT_STREQ(parser.backend(), "yaml-cpp");
}

TEST(YamlScenarioParser, ExampleFilesAreInTheTree) {
    for (const char* name : {"air_data_smoke.yaml", "engine_drop.yaml"}) {
        const std::filesystem::path path = scenario_path(name);
        std::ifstream in{path};
        ASSERT_TRUE(in.is_open()) << path;
        const std::string text((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
        EXPECT_NE(text.find("\nname:"), std::string::npos) << path;
    }
}

TEST(YamlScenarioParser, DISABLED_ParsesInlineSend) {
    constexpr std::string_view kYaml = R"(name: inline
steps:
  - name: ping
    requirement: REQ-1
    send:
      id: 256
      data: [1, 2]
)";
    Scenario scenario;
    scenario.name = "untouched";
    YamlScenarioParser parser;
    ASSERT_TRUE(parser.parse(kYaml, scenario).ok());

    EXPECT_EQ(scenario.name, "inline");
    EXPECT_TRUE(scenario.description.empty());
    EXPECT_EQ(scenario.bus, "loopback");
    EXPECT_TRUE(scenario.subsystems.empty());
    ASSERT_EQ(scenario.steps.size(), 1u);
    EXPECT_EQ(scenario.steps[0].kind, StepKind::Send);
    EXPECT_EQ(scenario.steps[0].name, "ping");
    EXPECT_EQ(scenario.steps[0].requirement_id, "REQ-1");
    EXPECT_EQ(scenario.steps[0].message.id, 256u);
    EXPECT_EQ(scenario.steps[0].message.data, bytes({1, 2}));
}

TEST(YamlScenarioParser, DISABLED_ParsesAirDataSmokeFile) {
    Scenario scenario;
    YamlScenarioParser parser;
    ASSERT_TRUE(parser.parse_file(scenario_path("air_data_smoke.yaml"), scenario).ok());

    EXPECT_EQ(scenario.name, "air-data-smoke");
    EXPECT_EQ(scenario.bus, "loopback");
    ASSERT_EQ(scenario.subsystems.size(), 1u);
    EXPECT_EQ(scenario.subsystems[0], "air-data");
    ASSERT_EQ(scenario.steps.size(), 2u);

    EXPECT_EQ(scenario.steps[0].kind, StepKind::Send);
    EXPECT_EQ(scenario.steps[0].name, "request indicated values");
    EXPECT_EQ(scenario.steps[0].requirement_id, "REQ-AD-001");
    EXPECT_EQ(scenario.steps[0].message.id, 256u);
    EXPECT_TRUE(scenario.steps[0].message.data.empty());

    EXPECT_EQ(scenario.steps[1].kind, StepKind::Expect);
    EXPECT_EQ(scenario.steps[1].requirement_id, "REQ-AD-001");
    EXPECT_EQ(scenario.steps[1].message.id, 257u);
    EXPECT_FALSE(scenario.steps[1].absent);
    EXPECT_EQ(scenario.steps[1].timeout, std::chrono::milliseconds{20});
    EXPECT_EQ(scenario.steps[1].message.data, bytes({100, 0, 50, 0}));
}

TEST(YamlScenarioParser, DISABLED_ParsesEngineDropFile) {
    Scenario scenario;
    YamlScenarioParser parser;
    ASSERT_TRUE(parser.parse_file(scenario_path("engine_drop.yaml"), scenario).ok());

    ASSERT_EQ(scenario.steps.size(), 3u);
    EXPECT_EQ(scenario.steps[0].kind, StepKind::Inject);
    EXPECT_EQ(scenario.steps[0].fault.kind, FaultKind::Drop);
    ASSERT_TRUE(scenario.steps[0].fault.match_id.has_value());
    EXPECT_EQ(*scenario.steps[0].fault.match_id, 513u);
    EXPECT_EQ(scenario.steps[0].fault.remaining, 1);

    EXPECT_EQ(scenario.steps[1].kind, StepKind::Send);
    EXPECT_EQ(scenario.steps[1].message.id, 512u);

    EXPECT_EQ(scenario.steps[2].kind, StepKind::Expect);
    EXPECT_TRUE(scenario.steps[2].absent);
    EXPECT_EQ(scenario.steps[2].message.id, 513u);
    EXPECT_EQ(scenario.steps[2].requirement_id, "REQ-ENG-001");
    EXPECT_EQ(scenario.steps[2].timeout, std::chrono::milliseconds{20});
}

TEST(YamlScenarioParser, DISABLED_RejectsMissingName) {
    Scenario scenario;
    scenario.name = "untouched";
    YamlScenarioParser parser;
    const aerotest::Status status = parser.parse("steps: []\n", scenario);
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(scenario.name, "untouched");
}

TEST(YamlScenarioParser, DISABLED_RejectsUnknownStep) {
    constexpr std::string_view kYaml = R"(name: bad
steps:
  - sleep: 1
)";
    Scenario scenario;
    scenario.name = "untouched";
    YamlScenarioParser parser;
    const aerotest::Status status = parser.parse(kYaml, scenario);
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(scenario.name, "untouched");
}

TEST(YamlScenarioParser, DISABLED_RejectsByteOver255) {
    constexpr std::string_view kYaml = R"(name: bad
steps:
  - send:
      id: 1
      data: [256]
)";
    Scenario scenario;
    scenario.name = "untouched";
    YamlScenarioParser parser;
    const aerotest::Status status = parser.parse(kYaml, scenario);
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(scenario.name, "untouched");
}

TEST(YamlScenarioParser, DISABLED_RejectsUnknownSubsystem) {
    constexpr std::string_view kYaml = R"(name: bad
subsystems:
  - radio
steps: []
)";
    Scenario scenario;
    scenario.name = "untouched";
    YamlScenarioParser parser;
    const aerotest::Status status = parser.parse(kYaml, scenario);
    EXPECT_EQ(status.code, Error::InvalidArgument);
    EXPECT_EQ(scenario.name, "untouched");
}

TEST(YamlScenarioParser, DISABLED_MissingFileLeavesScenarioUnchanged) {
    Scenario scenario;
    scenario.name = "untouched";
    YamlScenarioParser parser;
    const aerotest::Status status = parser.parse_file("/no/such/aerotest-scenario.yaml", scenario);
    EXPECT_EQ(status.code, Error::Io);
    EXPECT_EQ(scenario.name, "untouched");
}
