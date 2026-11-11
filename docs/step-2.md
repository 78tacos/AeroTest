# Step 2 — what to implement

Step 1 is the bus. Step 2 is a YAML test executive that drives that bus. The parser, the two sensor nodes, the runner, and the result writers are stubs. `backend()`, the node names and default values, `add_subsystem`, and `ScenarioResult::passed` / `failure_count` already have bodies. Leave those.

Search for the new stubs:

```bash
grep -n 'TODO(step-2)' src include
```

Step 1's stubs are still `TODO(step-1)`. Do not implement them in a step-2 change unless a task below says that task cannot go green without one.

## Depends on step 1

Some of these tests fail today for two reasons: the step-2 body is a stub, and the step-1 body it calls is also a stub. Enable a test only when both sides are real, or it fails for the earlier reason and tells you nothing about the code you just wrote.

| Step 2 task | Also needs | Why |
| --- | --- | --- |
| T0, T1, T2 | nothing unfinished | Parsing and printing do not send a frame |
| T3 `ClosedBusIsNotOpen` | `is_open()`, already done | A closed loopback is detectable today |
| T3 reply tests, `IgnoresOtherIds` | step 1 T3 and T4 | The node replies with `send`, and the test reads it back with `receive` |
| T4 `RefusesAClosedBus`, `RefusesAMismatchedBusName` | nothing unfinished | They return before any send |
| T4 `WaitRecordsAPass` | step 1 T3 | The runner refuses a closed bus, so `open` has to work. The wait itself does not send |
| T4 `AirDataSmokeRecordsTheRequirement` | step 1 T3, T4, and T6, plus T3 here | The expect step calls `TestRunner::expect_message` |
| T4 `EngineDropPassesWhenTheReplyIsAbsent` | step 1 T3, T4, and T5, plus T3 here | The reply has to be sent and then dropped. This expect does not use the test runner |
| T5 | T1, T2, T4, and whatever those need | The CLI only connects the pieces |

T1 and T2 can go green while step 1 is still stubs. That is deliberate.

## How the tests work

Same rule as [step 1](step-1.md). `DISABLED_` tests are compiled and skipped, so `ctest --preset gcc-debug` stays green. Remove the prefix, rebuild, and run. Rebuild before you trust CTest: discovery runs at build time.

```bash
./build/gcc-debug/tests/aerotest_tests \
  --gtest_also_run_disabled_tests \
  --gtest_filter='YamlScenarioParser.DISABLED_ParsesInlineSend'
```

If this file and a test disagree, the test is the acceptance check. Update this file in the same change.

Do not add a CTest for `aerotest run` until that command exits 0. `PASS_REGULAR_EXPRESSION` still requires a successful exit, and the stub exits 2 on purpose.

## Filing a task as an issue

Title: the task heading (`T1 — parse a scenario`).

Body: the task section, including the test names and the step-1 dependency. One task per issue.

## Suggested order

| Task | You end up able to |
| --- | --- |
| [T0](#t0--build-and-read-a-scenario) | Configure with yaml-cpp and read a scenario file |
| [T1](#t1--parse-a-scenario) | Turn YAML into a `Scenario` |
| [T2](#t2--write-json-and-junit) | Print a result with no bus involved |
| [T3](#t3--air-data-and-engine-nodes) | Have two nodes answer a request |
| [T4](#t4--run-a-scenario) | Play steps on the loopback bus |
| [T5](#t5--wire-the-cli) | Run a file from the command line |

## T0 — build and read a scenario

The first configure downloads yaml-cpp 0.9.0 as well as GoogleTest. The library links `yaml-cpp::yaml-cpp` privately. Public headers do not include it.

**Files:** `CMakeLists.txt`, `scenarios/air_data_smoke.yaml`, `scenarios/engine_drop.yaml`, `src/scenario_parser.cpp`

**Tests already enabled:**

- `YamlScenarioParser.BackendIsYamlCpp`
- `YamlScenarioParser.ExampleFilesAreInTheTree`

`backend()` constructs an empty yaml-cpp node and returns `"yaml-cpp"`. That is the proof the dependency is linked. It is not a parser.

**What you'll learn:** FetchContent for a second library, and the difference between linking a dependency and calling it.

**Done when:** both presets configure, build, and `ctest` still passes. You can point at the two scenario files and say what each step is asking for.

## T1 — parse a scenario

The contract is the comment on `YamlScenarioParser` and the two files under `scenarios/`. Short version:

- Ids and bytes are decimal. `256` is CAN id `0x100`. Do not accept hex.
- `name` is required and non-empty. `steps` is required. An empty step list is legal.
- `bus` defaults to `loopback`. An explicit empty string is invalid.
- Subsystem names are only `air-data` and `engine`.
- A step has exactly one of `send`, `expect`, `inject`, `wait`, plus optional `name` and `requirement`.
- A missing step name becomes `"send"`, `"expect"`, `"inject"`, or `"wait"`.
- `expect.timeout_ms` is required and greater than zero. `data` is required unless `absent: true`.
- `inject.remaining` uses the same rules as `FaultSpec`: `0` and values below `-1` are invalid.
- A data byte outside `0..255` is invalid.
- On any failure, return `InvalidArgument` and leave `out` unchanged. Build a local `Scenario` and assign it at the end.
- `parse_file`: a missing or unreadable file is `Error::Io` and does not call `parse`. Then it is just `parse`.

`YAML::Load` throws on malformed YAML. Catch that and return `InvalidArgument`. Do not let the exception out.

**Files:** `include/aerotest/scenario.hpp`, `include/aerotest/scenario_parser.hpp`, `src/scenario_parser.cpp`

**Tests** in `tests/scenario_parser_test.cpp`. None of them open a bus:

- `DISABLED_ParsesInlineSend`
- `DISABLED_ParsesAirDataSmokeFile`
- `DISABLED_ParsesEngineDropFile`
- `DISABLED_RejectsMissingName`
- `DISABLED_RejectsUnknownStep`
- `DISABLED_RejectsByteOver255`
- `DISABLED_RejectsUnknownSubsystem`
- `DISABLED_MissingFileLeavesScenarioUnchanged`

**What you'll learn:** loading a YAML document and mapping it into your own structs, and failing the whole document instead of returning a half-parsed one.

**Done when:** those eight tests pass with the prefix removed. Step 1 can still be stubs.

## T2 — write JSON and JUnit

Build a `ScenarioResult` in the test and print it. No bus, no parser, no runner.

The exact bytes are the strings in the tests. The header on `ResultsWriter` says the same thing in prose:

- JSON is one object, one trailing newline, keys in the order `name`, `passed`, `checks`, and inside each check `name`, `passed`, `requirement`, `detail`. No extra spaces. `passed` is a JSON boolean.
- Escape `\`, `"`, and newlines inside strings.
- JUnit is the shape in the header. A passing check is a self-closing `testcase`. A failing check has a `<failure message="failed">` child whose body is the detail. Escape `&`, `<`, `>`, and `"` in the name, the requirement id, and the detail.

`tests` is the number of checks. `failures` is `failure_count()`, which is already written.

This is not `ctest --output-junit`. That flag prints CTest's own results. These writers print one scenario run.

**Files:** `include/aerotest/results_writer.hpp`, `src/results_writer.cpp`

**Tests** in `tests/results_writer_test.cpp`:

- `DISABLED_PassIsOneObject`
- `DISABLED_EscapesQuoteBackslashAndNewline`
- `DISABLED_PassAndFailure`

`ScenarioResult.CountsLikeTheReport` is already enabled. Leave it.

**What you'll learn:** a text format as a contract, including the characters that would otherwise break it.

**Done when:** the three tests pass with the prefix removed. Still no step-1 dependency.

## T3 — air-data and engine nodes

Two nodes, one `on_frame` each. They do not own a socket. The runner will hand them the bus.

Reply only to the node's request id. Any other id returns Ok and sends nothing. The request's bytes are not yours to edit.

The request id on a closed bus returns `NotOpen` and sends nothing. `is_open()` is already real, so `ClosedBusIsNotOpen` does not need step 1's send.

The reply is four bytes, little-endian uint16. The headers spell the defaults:

| Node | Request | Reply | Default bytes |
| --- | --- | --- | --- |
| air-data | `0x100` (256) | `0x101` (257) | altitude 100, airspeed 50 → `100, 0, 50, 0` |
| engine | `0x200` (512) | `0x201` (513) | rpm 2400, coolant 90 → `96, 9, 90, 0` |

`2400` is `0x0960`, so the low byte is `96` and the next byte is `9`.

**Files:** `include/aerotest/subsystem.hpp`, `include/aerotest/air_data_subsystem.hpp`, `include/aerotest/engine_subsystem.hpp`, `src/air_data_subsystem.cpp`, `src/engine_subsystem.cpp`

**Tests** in `tests/subsystem_test.cpp`:

- `DISABLED_RepliesLittleEndian` (air-data and engine)
- `DISABLED_IgnoresOtherIds` (air-data and engine)
- `AirDataSubsystem.DISABLED_ClosedBusIsNotOpen`

`NameAndDefaults` on both nodes is already enabled.

**What you'll learn:** a node that answers one CAN id, and why the byte order has to be written down once.

**Done when:** the closed-bus test passes, and the reply tests pass after step 1 T3 and T4.

## T4 — run a scenario

`ScenarioRunner::run` plays a `Scenario` on a bus the caller already opened. The comment on the class is the contract. The order that is easy to get wrong:

1. If `scenario.bus` is not `bus.name()`, return `InvalidArgument` and do not touch `out`. A closed bus does not change this.
2. If the bus is not open, return `NotOpen` and do not touch `out`.
3. If a subsystem name in the scenario was not passed to `add_subsystem`, return `InvalidArgument` and do not touch `out`.
4. `set_fault_injector` with the injector the runner was given.
5. Each step appends one check, then the next step runs. A failed check does not abort the scenario. `run` returns Ok when the list is finished. The caller uses `out.passed()`.

Send, then show that same frame to each named subsystem. The node replies with `bus.send`, which lands on the loopback queue. The following expect receives it. `wait` is the only step that sleeps. Loopback `expect` does not block: a Delay fault needs a `wait` before the expect, or the frame is still in the future and the expect times out.

An expect with `absent: false` calls `TestRunner::expect_message` and then copies the returned check, setting `requirement_id` from the step. Do not also copy `report().checks`. Step 1 leaves the id empty on purpose.

An expect with `absent: true` calls `bus.receive` itself. Timeout is a pass. A frame is a fail. `expect_message` would record the timeout as a failure, which is the wrong answer for this step.

Switch on `StepKind` with no default arm that returns a value, so a fifth kind fails the build until you handle it.

**Files:** `include/aerotest/scenario_runner.hpp`, `src/scenario_runner.cpp`

**Tests** in `tests/scenario_runner_test.cpp`. They build a `Scenario` in C++, so they do not need T1:

- `DISABLED_RefusesAClosedBus`
- `DISABLED_RefusesAMismatchedBusName`
- `DISABLED_WaitRecordsAPass` (needs step 1 T3)
- `DISABLED_AirDataSmokeRecordsTheRequirement` (needs step 1 T3, T4, T6, and T3 here)
- `DISABLED_EngineDropPassesWhenTheReplyIsAbsent` (needs step 1 T3, T4, T5, and T3 here)

`SlotStartsEmpty` is already enabled.

**What you'll learn:** a pump with a fixed order, and which step is allowed to sleep.

**Done when:** the two refuse-tests pass now, the wait test passes after step 1 T3, and the two scenario tests pass after the step-1 tasks named above.

## T5 — wire the CLI

`aerotest run <file>` already returns 2 and names the file. Replace that stub.

- Parse the file (T1).
- Open a `LoopbackBus` when `scenario.bus` is `loopback`. Any other bus name is exit 2 for now. SocketCAN from the CLI is not this task.
- Add the subsystems the scenario names, in that order.
- `run` (T4).
- Write `<stem>.json` and `<stem>.xml` in the current directory (T2). `air_data_smoke.yaml` writes `air_data_smoke.json` and `air_data_smoke.xml`.
- Exit 0 when `result.passed()`. Exit 1 when the run finished and a check failed. Exit 2 on usage, a parse error, a missing file, or a `run` status that is not Ok.

There is no CTest for this until the success path exits 0. Check it by hand:

```bash
./build/gcc-debug/aerotest run scenarios/air_data_smoke.yaml
./build/gcc-debug/aerotest run scenarios/engine_drop.yaml
./build/gcc-debug/aerotest run missing.yaml
```

The help text must keep the substring `docs/step-1.md`. `cli.help` and `cli.no_args` lock it.

**Files:** `apps/aerotest_cli/main.cpp`

**What you'll learn:** a `main` that only connects pieces the tests already cover.

**Done when:** the two example scenarios exit 0 and write both files, a bad path exits 2, and the existing CLI tests still pass.

## Done means

- T1 and T2 pass with their `DISABLED_` prefixes removed, even if step 1 is still stubs.
- T3 and T4 pass after the step-1 tasks named above.
- `ctest --preset gcc-debug` and `ctest --preset clang-debug` stay green, and the `needs-vcan` label stays excluded.
- The only new dependency is yaml-cpp. No Qt, no `spdlog`, no JSON library.
- Nothing in the README claims certification, DO-178C, or flight software.
