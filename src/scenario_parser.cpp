// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/scenario_parser.hpp"

#include <yaml-cpp/yaml.h>

namespace aerotest {

const char* YamlScenarioParser::backend() const {
    // Touch a node so this translation unit cannot build without yaml-cpp.
    // Parsing is task T1 in docs/step-2.md.
    const YAML::Node probe;
    return probe.IsNull() ? "yaml-cpp" : "yaml-cpp-node";
}

Status YamlScenarioParser::parse(std::string_view yaml, Scenario& out) {
    // TODO(step-2): task T1 in docs/step-2.md.
    // Load the text, fill a local Scenario, and assign `out` only after the
    // whole document is valid. Any problem returns InvalidArgument and leaves
    // `out` unchanged.
    (void)yaml;
    (void)out;
    return Status::not_implemented("YamlScenarioParser::parse");
}

Status YamlScenarioParser::parse_file(const std::filesystem::path& path, Scenario& out) {
    // TODO(step-2): task T1 in docs/step-2.md.
    // A missing or unreadable file is Error::Io and does not call parse.
    (void)path;
    (void)out;
    return Status::not_implemented("YamlScenarioParser::parse_file");
}

} // namespace aerotest
