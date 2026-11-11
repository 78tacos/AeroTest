// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#include "aerotest/version.hpp"

#include <iostream>
#include <string_view>

namespace {

void print_help() {
    std::cout << "aerotest " << aerotest::kVersion
              << "\n"
                 "Educational CAN bus simulator. Not certified. Not flight software.\n"
                 "\n"
                 "Usage:\n"
                 "  aerotest --help\n"
                 "  aerotest --version\n"
                 "\n"
                 "Step 1 does not run scenarios yet. The task list is docs/step-1.md.\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "aerotest: unexpected argument '" << argv[2] << "'\n";
        return 2;
    }

    const std::string_view argument =
        argc > 1 ? std::string_view{argv[1]} : std::string_view{"help"};
    if (argument == "--help" || argument == "-h" || argument == "help") {
        print_help();
        return 0;
    }
    if (argument == "--version" || argument == "-V" || argument == "version") {
        std::cout << "aerotest " << aerotest::kVersion << '\n';
        return 0;
    }

    std::cerr << "aerotest: unknown command '" << argument << "'\n";
    std::cerr << "Try 'aerotest --help'.\n";
    return 2;
}
