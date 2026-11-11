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
                 "  aerotest run <scenario.yaml>\n"
                 "\n"
                 "The task lists are docs/step-1.md and docs/step-2.md.\n";
}

} // namespace

int main(int argc, char** argv) {
    // `run` is the only command that takes a second argument. It is recognized
    // so the usage is real. The body is task T5 in docs/step-2.md.
    if (argc >= 2 && std::string_view{argv[1]} == "run") {
        if (argc != 3) {
            std::cerr << "aerotest: usage: aerotest run <scenario.yaml>\n";
            return 2;
        }
        std::cerr << "aerotest: scenario runner is not implemented yet (" << argv[2]
                  << "). See docs/step-2.md\n";
        return 2;
    }

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
