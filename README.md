# AeroTest

Educational C++20 avionics bus simulator and test-automation platform.

Step 1 is a classical CAN stack: a frame type, a `Bus` interface, an in-memory loopback bus, a SocketCAN / vCAN bus, fault-injection hooks, and a small test runner that records each check. Step 2 is a YAML scenario runner on that bus, two synthetic sensor nodes, a requirement id on each check, and JSON / JUnit results. Later steps add a MIL-STD-1553 subset, a fuller trace matrix, and richer reports.

This is a portfolio project by [78tacos](https://github.com/78tacos). The SocketCAN shape follows lessons from [CANServer](https://github.com/78tacos/CANServer), the C++20 TCP scheduler from the SIUE capstone CANtrol Center. AeroTest does not reuse that code. CANServer is one translation unit, a Makefile, and a text protocol that can shell out to `cansend`. AeroTest is a library with an interface, a loopback double, and tests aimed at the bus itself. The raw socket is the only transmit path.

The behavior behind the stubs is the work in [docs/step-1.md](docs/step-1.md) and [docs/step-2.md](docs/step-2.md). Methods that already have a body are part of the scaffold. Anything marked `TODO(step-1)` or `TODO(step-2)` returns `Error::NotImplemented` on purpose.

## Disclaimer

AeroTest is educational software. It is not certified, it is not DO-178C, and it is not flight software. It does not contain export-controlled data. Frames in tests and examples are synthetic. Do not attach it to an aircraft, a vehicle, or other safety-critical equipment.

## Roadmap

| Step | What it adds |
| --- | --- |
| 1 | Classical CAN message, `Bus`, loopback, SocketCAN / vCAN, fault injection, a recorded test runner. The logic is still [docs/step-1.md](docs/step-1.md) |
| 2 (this tree's new work) | YAML scenarios, two synthetic nodes, a requirement id on each check, JSON and JUnit results. The logic is still [docs/step-2.md](docs/step-2.md) |
| 3 | MIL-STD-1553 subset (bus controller, remote terminal, bus monitor) with its own frame type |
| 4 | Traceability beyond a single id on one check |
| 5 | Reports richer than the JSON and JUnit from step 2 |
| Later | ARINC 429, an optional Qt UI, `spdlog` for logging |

YAML is step 2, ahead of 1553, because the executive drives the step-1 `Bus`. A second bus can reuse the same scenario file later. Step 2 starts traceability and reports: one requirement id on each check, and two result files. A matrix across runs, and a report richer than those files, stay steps 4 and 5.

`yaml-cpp` 0.9.0 is a dependency because the parser links it. `parse` is still a stub. `spdlog` and Qt are not dependencies. Adding them unused would only hide which step they belong to.

## Layout

```
include/aerotest/     public headers
src/                  library sources
apps/aerotest_cli/    CLI stub (help, version, and `run`)
scenarios/            example YAML scenarios
tests/                GoogleTest specs, plus a separate vCAN binary
docs/step-1.md        the CAN implementation list
docs/step-2.md        the scenario implementation list
docs/cmake.md         the CMake files, line by line
```

| Header | Role |
| --- | --- |
| `status.hpp` | `Error` and `Status` |
| `message.hpp` | One classical CAN frame, and `validate()` |
| `bus.hpp` | `open` / `send` / `receive` / `close`, plus a non-owning fault-injector slot |
| `loopback_bus.hpp` | In-memory bus the unit tests drive |
| `socketcan_bus.hpp` | Linux CAN socket. `is_open()` is "`socket_fd_ >= 0`" |
| `fault_injector.hpp` | Drop, corrupt, delay, and error faults |
| `test_runner.hpp` | Send a frame, compare what came back, record the check |
| `scenario.hpp` | One parsed scenario: send, expect, inject, wait |
| `scenario_parser.hpp` | YAML text in, `Scenario` out. The header does not include yaml-cpp |
| `subsystem.hpp` | A node that answers a frame on a bus it does not own |
| `air_data_subsystem.hpp` | Synthetic air-data request `0x100`, reply `0x101` |
| `engine_subsystem.hpp` | Synthetic engine request `0x200`, reply `0x201` |
| `scenario_runner.hpp` | Play a scenario on a bus the caller already opened |
| `results_writer.hpp` | JSON and JUnit for one `ScenarioResult` |
| `version.hpp` | Generated from `project(VERSION)` |

`Message` is a CAN frame. A 1553 command word is a different type and lands in a later step behind the same idea (open, send, receive, close), not inside this struct.

## What already runs

These are finished, and the enabled tests lock them in:

- The data fields on `Message`, `Status`, `FaultSpec`, and `CheckResult`
- `LoopbackBus::name()`, `is_open()`, `queued()`, and the fault-injector slot
- `SocketCanBus`'s stored interface name, and `is_open()` from the file descriptor
- `FaultInjector::clear()` and `pending()`
- `TestRunner::Report::passed()` and `failure_count()`, and the empty default of `CheckResult::requirement_id`
- `ScenarioResult::passed()` and `failure_count()`
- `YamlScenarioParser::backend()`, which only proves yaml-cpp is linked
- The air-data and engine node names and their default published values
- `ScenarioRunner::add_subsystem`
- The CLI's `--help` and `--version`

`validate`, `to_string`, loopback transfer, fault decisions, the runner's checks, and every SocketCAN system call are still step-1 stubs. Parsing a scenario, answering a request, playing steps, and writing JSON or JUnit are step-2 stubs.

## Build

Ubuntu 24.04, CMake 3.25 or newer, Ninja, and a C++20 compiler. The first configure downloads yaml-cpp 0.9.0, and GoogleTest 1.18.0 when tests are on, so it needs a network.

```bash
sudo apt update
sudo apt install ninja-build cmake g++ clang
```

GCC:

```bash
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug --output-on-failure
```

Clang:

```bash
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

`cmake --list-presets` prints the names. `gcc-asan` is the GCC debug preset plus AddressSanitizer and UndefinedBehaviorSanitizer. CI does not run it.

```bash
./build/gcc-debug/aerotest --help
./build/gcc-debug/aerotest --version
```

If `clang++` compiles and then fails with `cannot find -lstdc++`, it has selected a newer GCC than `/usr/bin/g++`. Install that version's development package (`libstdc++-14-dev` when the selected install ends in `/14`). CI does this for you. Details are in [docs/cmake.md](docs/cmake.md).

Build trees live under `build/` and are gitignored.

## Tests

Enabled tests cover the scaffold. They are supposed to pass today.

Tests for behavior that is still a stub are named `DISABLED_...` in the source. GoogleTest compiles them and skips them. CTest discovers them, drops the `DISABLED_` prefix from the displayed name, and marks them disabled, so a normal `ctest` run stays green. That is the point: the assertions are the assignment, and CI does not fail on unfinished work.

Turn one test on by renaming it, then rebuild so CTest's discovery list updates:

```cpp
TEST(LoopbackBus, DISABLED_RoundTrip)  // skipped
TEST(LoopbackBus, RoundTrip)           // runs
```

Run a single disabled test without renaming it:

```bash
./build/gcc-debug/tests/aerotest_tests \
  --gtest_also_run_disabled_tests \
  --gtest_filter='LoopbackBus.DISABLED_RoundTrip'
```

It should fail until the body is real. A disabled test that already passes is not a target; the assertion is too weak, or the stub accidentally implements it.

`tests/vcan_bus_test.cpp` is a second binary, labeled `needs-vcan`. The presets exclude that label, including after you remove `DISABLED_`, because GitHub-hosted CI has no `vcan0`. Bring the interface up and opt in:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan   # harmless to skip if it already exists
sudo ip link set up vcan0
ctest --test-dir build/gcc-debug -L needs-vcan --output-on-failure
```

`can-utils` is not required. Nothing in this tree calls `cansend`.

Which tests belong to which task is listed in [docs/step-1.md](docs/step-1.md) and [docs/step-2.md](docs/step-2.md). Step-2 tests that drive the bus stay `DISABLED_` until the step-1 task they call is real. The dependency table is at the top of step 2.

Warnings are on (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion`). `-Werror` is the `AEROTEST_WERROR` option and defaults off, so a noisy line does not block the build while you are learning. Turn it on for a task when you want a missing `enum` case to fail the build.

## License

[MIT](LICENSE).
