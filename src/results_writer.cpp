// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/results_writer.hpp"

namespace aerotest {

Status JsonResultsWriter::write(const ScenarioResult& result, std::ostream& out) const {
    // TODO(step-2): task T2 in docs/step-2.md.
    // One JSON object, the key order in the header, a trailing newline.
    (void)result;
    (void)out;
    return Status::not_implemented("JsonResultsWriter::write");
}

Status JunitResultsWriter::write(const ScenarioResult& result, std::ostream& out) const {
    // TODO(step-2): task T2 in docs/step-2.md.
    // The XML shape in the header, including the escapes.
    (void)result;
    (void)out;
    return Status::not_implemented("JunitResultsWriter::write");
}

} // namespace aerotest
