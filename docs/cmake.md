# The CMake files

AeroTest's build is three files: the top-level `CMakeLists.txt`, `tests/CMakeLists.txt`, and `CMakePresets.json`. CI in `.github/workflows/ci.yml` only calls the presets. This page walks each of them in order. The comments in the files are the short version of the same ideas.

CANServer builds one `.cpp` with a Makefile. That fits a single translation unit. This tree is a library, a CLI, and two test binaries, compiled with GCC and Clang. The preset names those combinations so you do not have to remember the flags.

## The cycle

```bash
cmake --preset gcc-debug          # configure: write build/gcc-debug
cmake --build --preset gcc-debug  # compile
ctest --preset gcc-debug          # run the enabled tests
```

Configure is the slow one the first time, because it downloads yaml-cpp, and GoogleTest when tests are on. Build and test are what you run after each edit. `cmake --list-presets` prints the names, including the descriptions from the JSON.

`build/` is gitignored. Delete a preset's directory if a configure gets stuck; the next `--preset` starts clean.

## Top-level `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.25)
```

Refuse to configure on an older CMake. 3.25 is the floor because `CMakePresets.json` uses preset version 6, and because `COMPILE_WARNING_AS_ERROR` (used when `AEROTEST_WERROR` is on) needs 3.24. Ubuntu 24.04's package is 3.28.

```cmake
project(AeroTest
  VERSION 0.1.0
  DESCRIPTION "Educational avionics bus simulator and test runner"
  LANGUAGES C CXX
)
```

`project` sets the project name and, through `VERSION`, the variables `PROJECT_VERSION`, `PROJECT_VERSION_MAJOR`, `PROJECT_VERSION_MINOR`, and `PROJECT_VERSION_PATCH`. The generated version header copies those. Bump `VERSION` and the header follows; there is no second number to edit.

`LANGUAGES C CXX` turns on both compilers. Our sources are C++. GoogleTest's own build still probes the C compiler, and the Clang preset sets `CMAKE_C_COMPILER` so that probe does not silently use GCC while `clang++` compiles the C++.

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

Every target created after these lines defaults to C++20. `REQUIRED` makes configure fail if the compiler cannot do it, instead of quietly falling back. `EXTENSIONS OFF` asks for `c++20` rather than `gnu++20`, so a GCC-only keyword is a real error.

`target_compile_features(... cxx_std_20)` later says the same thing on the library itself. The global `set` covers the CLI and the tests. The target command travels with `aerotest` if some other project links it. Keeping both is deliberate: the global lines are the project default, the target line is the library's own requirement.

```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

Write `compile_commands.json` into the build directory. `clangd` and `clang-tidy -p build/gcc-debug` read it. A hand-written `cmake -S . -B build` gets the file too, because this line lives in the project rather than only in a preset.

```cmake
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug or Release" FORCE)
endif()
```

Ninja is a single-config generator. If nobody sets a build type, you get an unoptimized binary with no debug symbols and no obvious reason. Presets set `CMAKE_BUILD_TYPE` in the cache before this line runs, so the `if` is false and the preset wins. The line only covers a bare `cmake -S . -B build`. `CMAKE_CONFIGURATION_TYPES` is how multi-config generators (Visual Studio, Xcode) say "we do not use `CMAKE_BUILD_TYPE`"; the check leaves them alone.

```cmake
option(AEROTEST_BUILD_TESTS "Build GoogleTest targets" ON)
option(AEROTEST_WARNINGS "Turn on a useful warning set" ON)
option(AEROTEST_WERROR "Treat warnings as errors" OFF)
option(AEROTEST_SANITIZERS "AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
```

`option` is a cache variable you can flip without editing the file: `cmake --preset gcc-debug -DAEROTEST_WERROR=ON`. The description string is what `cmake -LH` shows. Tests default on. Warnings-as-errors default off so a learning build is not blocked by a noisy line. Turn `AEROTEST_WERROR` on for task T1 if you want a missing `enum` case to fail the build.

```cmake
if(AEROTEST_SANITIZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "AEROTEST_SANITIZERS supports GCC and Clang only")
  endif()
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()
```

`add_compile_options` and `add_link_options` apply to this directory and to subdirectories, which includes the GoogleTest that `tests/CMakeLists.txt` fetches. The flags have to be on gtest too. Instrumenting only our targets and then linking an unsanitized gtest fails at link time. `message(FATAL_ERROR ...)` stops configure with that text.

The `gcc-asan` preset sets `AEROTEST_SANITIZERS` to `ON`. CI does not build that preset.

```cmake
function(aerotest_set_warnings target)
  if(NOT AEROTEST_WARNINGS)
    return()
  endif()
  target_compile_options(${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion>
  )
  if(AEROTEST_WERROR)
    set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
  endif()
endfunction()
```

A function so the library, the CLI, and both test binaries share one warning set. `PRIVATE` means the flags apply when compiling that target, and they are not pushed onto a later target that only links it.

The `$<...>` string is a generator expression, evaluated at generate time per compiler. `OR` is true for GCC or Clang. The semicolons are required: CMake splits them into separate compiler arguments (`-Wall`, then `-Wextra`, ...). Spaces would be one argument and the compiler would reject it.

`COMPILE_WARNING_AS_ERROR` is CMake's portable spelling of `-Werror`. It is a target property, so gtest does not inherit it.

```cmake
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/aerotest/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/aerotest/version.hpp
  @ONLY
)
```

Copy the `.in` file into the build tree, replacing `@PROJECT_VERSION@` and the three component variables. `@ONLY` means `${...}` is left untouched; only `@VAR@` is substituted. The output path ends in `aerotest/version.hpp` so the include is `#include "aerotest/version.hpp"`, the same shape as the hand-written headers.

Do not edit the generated file. The next configure overwrites it. `clang-format` wants to insert a space in `@PROJECT_VERSION_MAJOR@`, which stops the substitution from matching, so that block in the `.in` file is wrapped in `clang-format off`.

```cmake
add_library(aerotest
  src/status.cpp
  ...
)
add_library(aerotest::aerotest ALIAS aerotest)
```

The sources are written out on purpose. `file(GLOB ...)` looks convenient and then ignores a `.cpp` you just added until something else forces a reconfigure. Adding a file is a one-line edit here, and the diff shows it. Step 2 adds `scenario_parser.cpp`, `air_data_subsystem.cpp`, `engine_subsystem.cpp`, `scenario_runner.cpp`, and `results_writer.cpp` to this list.

```cmake
include(FetchContent)
FetchContent_Declare(
  yaml-cpp
  URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.9.0.tar.gz
  URL_HASH SHA256=25cb043240f828a8c51beb830569634bc7ac603978e0f69d6b63558dadefd49a
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_CPP_FORMAT_SOURCE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)
```

Same mechanism as GoogleTest, placed in the top-level file because the library links it, not only the tests. The URL is the 0.9.0 tag tarball. `URL_HASH` fails the configure if those bytes change.

The five `set(... CACHE BOOL "" FORCE)` lines run *before* `FetchContent_MakeAvailable`, which is what parses yaml-cpp's `CMakeLists.txt`. yaml-cpp defaults to building a small command-line tool and its contrib sources. Both are off here. Its own tests are already off when it is not the top-level project; the `FORCE` keeps a stale cache from turning them back on. Install rules and a clang-format target are off so a configure of AeroTest does not grow extra targets.

```cmake
target_link_libraries(aerotest PRIVATE yaml-cpp::yaml-cpp)
```

`yaml-cpp::yaml-cpp` is the namespaced alias, same pattern as `aerotest::aerotest`. `PRIVATE` means the parser's `.cpp` can include `yaml-cpp/yaml.h`, and a target that links `aerotest` does not gain that include path. The static library still pulls yaml-cpp onto the final link line, so the CLI and the tests resolve the yaml-cpp symbols without including the header. Public headers stay free of it. That is why a missing yaml include fails in `scenario_parser.cpp` and nowhere else.

Two follow-up lines exist because yaml-cpp is a real project, not a header.

```cmake
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set_target_properties(yaml-cpp PROPERTIES CXX_SCAN_FOR_MODULES OFF)
```

CMake 3.28 can scan C++20 sources for modules. This tree has none. Clang's scanner is a separate `clang-scan-deps` binary that `apt install clang` does not provide, and the build then fails with `CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS-NOTFOUND`. The variable is set off before the fetch. yaml-cpp calls `project()` in a subdirectory, which turns the scan back on, so it is set off again afterwards and the property is pinned on the `yaml-cpp` target it already created. Our `add_library` comes after that second `set`, which is why the order matters.

```cmake
get_target_property(aerotest_yaml_includes yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(yaml-cpp PROPERTIES
  INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${aerotest_yaml_includes}"
)
```

yaml-cpp 0.9's headers warn under Clang's `-Wsign-conversion` (GCC's `-Wconversion` does not include that one). A system include directory suppresses warnings from those headers, so turning on `AEROTEST_WERROR` does not make a third-party header fail our build. The path is read back from the target instead of being typed out.

The `ALIAS` is the name tests link (`aerotest::aerotest`). The `::` makes a missing target a configure error. A typo of a bare name can quietly become a linker flag instead.

```cmake
target_include_directories(aerotest PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)
```

`PUBLIC` means: when compiling `aerotest`, and when compiling anything that links it. `BUILD_INTERFACE` limits the path to users of this build tree (the CLI and the tests). There is no install rule in step 1, so there is no `INSTALL_INTERFACE` path yet.

The second path is how `#include "aerotest/version.hpp"` finds the generated header.

```cmake
target_compile_features(aerotest PUBLIC cxx_std_20)
aerotest_set_warnings(aerotest)
```

The feature requirement is `PUBLIC`, so a target that links the library is also compiled as C++20. The warnings stay inside the function, which uses `PRIVATE`, so linking the library does not turn `-Wconversion` on in somebody else's `.cpp`. Our CLI and tests call `aerotest_set_warnings` themselves.

```cmake
add_executable(aerotest_cli apps/aerotest_cli/main.cpp)
set_target_properties(aerotest_cli PROPERTIES OUTPUT_NAME aerotest)
target_link_libraries(aerotest_cli PRIVATE aerotest::aerotest)
aerotest_set_warnings(aerotest_cli)
```

The CMake target is `aerotest_cli` so it does not collide with the library target `aerotest`. `OUTPUT_NAME` is the filename you run: `build/gcc-debug/aerotest`. `PRIVATE` link means the CLI uses the library and does not re-export it.

```cmake
if(AEROTEST_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
```

`enable_testing()` turns on CTest in this directory. Without it, `add_test` in the subdirectory is ignored. `add_subdirectory` processes `tests/CMakeLists.txt` with all of the functions defined above still visible. CMake functions are available to subdirectories; that is why `aerotest_set_warnings` works down there.

## `tests/CMakeLists.txt`

```cmake
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
  URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
)
```

`FetchContent` downloads a dependency at configure time and adds its CMake project to this build. The URL is the release tarball, pinned to v1.18.0. `URL_HASH` makes configure fail if the bytes change, instead of building whatever the URL happens to serve later. The tarball lands under `build/<preset>/_deps/`. The first configure needs a network; later configures reuse the download.

```cmake
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
```

These three are GoogleTest's own options. They have to be set in the cache *before* `FetchContent_MakeAvailable`, because that command is what runs gtest's `CMakeLists.txt`. `FORCE` overwrites gtest's default if a previous configure cached the other value.

- `gtest_force_shared_crt` matters on Windows, where gtest otherwise picks a static CRT and then refuses to link. Harmless on Linux.
- `INSTALL_GTEST` off: we are not installing gtest when someone installs AeroTest.
- `BUILD_GMOCK` off: the tests use GoogleTest, not GoogleMock. Skipping gmock shortens the build. The `GTest::gtest_main` target still exists; it supplies `main()`.

```cmake
add_executable(aerotest_tests
  version_test.cpp
  ...
)
target_link_libraries(aerotest_tests PRIVATE aerotest::aerotest GTest::gtest_main)
aerotest_set_warnings(aerotest_tests)
```

One test binary for everything that does not need a CAN interface, including the step-2 tests. `GTest::gtest_main` is the namespaced target FetchContent creates (an `ALIAS` inside GoogleTest, same pattern as `aerotest::aerotest`). Linking it pulls in gtest and defines `main`. Test files do not write their own `main`.

```cmake
target_compile_definitions(aerotest_tests PRIVATE
  "AEROTEST_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\""
)
```

`CMAKE_SOURCE_DIR` is the source root, the directory that contains `scenarios/`. The quotes are part of the compiler definition, so the test sees a string literal. The parser tests open `scenarios/air_data_smoke.yaml` from that path. The library does not get the definition: a shipped binary should not bake in a build-machine path. The CLI takes the path as an argument instead.

```cmake
include(GoogleTest)
gtest_discover_tests(aerotest_tests
  PROPERTIES LABELS "step1"
  DISCOVERY_TIMEOUT 30
)
```

`include(GoogleTest)` loads CMake's module, not the library. `gtest_discover_tests` runs the test binary after it is built, with `--gtest_list_tests`, and registers each test with CTest. A test whose GoogleTest name contains `DISABLED_` is registered as disabled. CTest drops that prefix in the name it prints, which is why the log says `LoopbackBus.RoundTrip` and `Not Run (Disabled)` while the source still says `DISABLED_RoundTrip`.

Discovery happens at build time. Renaming a test and running `ctest` without rebuilding leaves CTest holding the old list.

`LABELS "step1"` tags every test in this binary, including the step-2 tests that live in the same executable. The label means "not the vCAN binary". The presets do not filter on it. `DISCOVERY_TIMEOUT 30` is how long CMake waits for `--gtest_list_tests`. The binary only prints names, so it finishes immediately; the timeout is there so a hung binary fails the build instead of hanging it.

```cmake
add_executable(aerotest_vcan_tests vcan_bus_test.cpp)
target_link_libraries(aerotest_vcan_tests PRIVATE aerotest::aerotest GTest::gtest_main)
aerotest_set_warnings(aerotest_vcan_tests)
gtest_discover_tests(aerotest_vcan_tests
  PROPERTIES LABELS "needs-vcan"
  DISCOVERY_TIMEOUT 30
)
```

A second binary so the kernel tests can carry a different label. They are still compiled in CI, which is what keeps `vcan_bus_test.cpp` honest. The test presets exclude `needs-vcan`, so `ctest --preset gcc-debug` does not run them. After you remove `DISABLED_` from those tests, you opt in with `ctest --test-dir build/gcc-debug -L needs-vcan`.

```cmake
add_test(NAME cli.version COMMAND $<TARGET_FILE:aerotest_cli> --version)
set_tests_properties(cli.version PROPERTIES
  LABELS "step1"
  PASS_REGULAR_EXPRESSION "aerotest 0\\.1\\.0"
)
```

`add_test` registers a command, not a GoogleTest case. `$<TARGET_FILE:aerotest_cli>` is the full path to the built CLI. Writing `COMMAND aerotest` would look the name up on `PATH` and fail. The same generator expression is how CTest finds a binary whose folder depends on the preset.

`PASS_REGULAR_EXPRESSION` requires that substring on stdout, and the process still has to exit 0. The backslashes escape the dots so `0.1.0` is matched literally. `cli.help` and `cli.no_args` require the task-list path, which is how the help text stays pointed at `docs/step-1.md`.

## `CMakePresets.json`

Presets are named configurations. Version 6 needs CMake 3.25, matching `cmake_minimum_required`. The file is JSON, so it has no comments; the `description` fields are what `cmake --list-presets` prints.

Each configure preset sets:

| Field | Purpose |
| --- | --- |
| `generator` | `Ninja`. One build tool for every preset. |
| `binaryDir` | `build/gcc-debug`, `build/clang-debug`, or `build/gcc-asan`. Two compilers do not share a directory; the cached compiler is part of that directory. |
| `CMAKE_BUILD_TYPE` | `Debug`. |
| `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER` | Both, so the C probe and the C++ compile use the same family. |

`gcc-asan` repeats the GCC preset and adds `AEROTEST_SANITIZERS=ON`. It is not in the CI matrix.

Build presets are one line each: `"configurePreset": "gcc-debug"` means `cmake --build --preset gcc-debug` knows which directory to build. There is no extra flag.

Test presets point at the same configure preset and add three behaviors:

- `outputOnFailure`: print the failing program's output. Passing tests stay quiet.
- `noTestsAction: error`: if discovery registered nothing, `ctest` fails. A silent "no tests" is a broken build, not a pass.
- `filter.exclude.label: needs-vcan`: skip the kernel tests. Disabled GoogleTest cases are already skipped; this line still matters the day those tests lose their `DISABLED_` prefix.

The three test presets are copies. Inheritance would hide the exclude rule in a hidden base preset. The repetition is so you can read one preset and see the whole thing.

## CI

`.github/workflows/ci.yml` runs on a push to `main`, on every pull request, and when someone starts it by hand (`workflow_dispatch`).

`permissions: contents: read` is the whole token the job gets. `concurrency` cancels a superseded run on the same branch.

The matrix is the two presets CI cares about: `gcc-debug` and `clang-debug`. `fail-fast: false` means a GCC failure does not cancel the Clang job.

The install step adds Ninja, CMake, `g++`, and `clang`. It then asks `clang++ -v` which GCC install it selected and installs that version's `libstdc++-N-dev`. On Ubuntu 24.04, Clang often selects GCC 14 while `/usr/bin/g++` is still GCC 13. The link then dies with `cannot find -lstdc++` until the matching `-dev` package is present. The same snippet is the fix on a laptop.

The next three steps are the same commands as the local cycle: `cmake --preset`, `cmake --build --preset`, `ctest --preset`.

## Adding a file

- A new library `.cpp`: add its path to `add_library(aerotest ...)` in the top-level file.
- A new unit test: add the `.cpp` to `aerotest_tests` in `tests/CMakeLists.txt`. A test that needs `vcan0` goes in `aerotest_vcan_tests` instead, or into `vcan_bus_test.cpp`.
- A new header under `include/aerotest/` needs no CMake edit. The include directory is already on the target.

Reconfigure after a `CMakeLists.txt` edit (`cmake --preset gcc-debug` again). A source edit only needs a rebuild.

## Knobs worth knowing

| Command | Effect |
| --- | --- |
| `cmake --preset gcc-debug -DAEROTEST_BUILD_TESTS=OFF` | Library and CLI only. No download of GoogleTest. yaml-cpp is still downloaded, because the library links it. |
| `cmake --preset gcc-debug -DAEROTEST_WARNINGS=OFF` | Compiles without the warning set. |
| `cmake --preset gcc-debug -DAEROTEST_WERROR=ON` | Warnings fail the build. Useful for T1. |
| `cmake --preset gcc-asan` | Address and undefined-behavior sanitizers, including inside GoogleTest. |

Changing an `option` on an existing build directory is a reconfigure, not a rebuild alone. If the cache seems to ignore you, pass the `-D` on the `cmake --preset` line rather than editing the JSON.
