# Step 1 — what to implement

The project configures, builds, and the enabled tests pass. The bus does not move a frame yet. Each task below is one change you can make, in order. The `.cpp` bodies marked `TODO(step-1)` are yours. Methods that already have a body in a header are the scaffold; leave them unless a task says otherwise.

Search for the marker when you want the full list of stubs:

```bash
grep -n TODO src include
```

## How the tests work

A test whose name starts with `DISABLED_` is compiled and skipped. CTest shows it without that prefix and marks it disabled, so `ctest --preset gcc-debug` stays green.

When you finish a task, delete the `DISABLED_` prefix from that task's tests, rebuild, and run them. Rebuilding matters: CTest learns the new names from a build step, not from a header you only saved.

Run one skipped test while you are still working on it:

```bash
./build/gcc-debug/tests/aerotest_tests \
  --gtest_also_run_disabled_tests \
  --gtest_filter='LoopbackBus.DISABLED_RoundTrip'
```

The filter uses the GoogleTest name (`Suite.DISABLED_Name`), which still has the prefix. After you remove the prefix, the same test is `LoopbackBus.RoundTrip` and CTest will run it.

If the prose in this file and a test disagree, the test is the acceptance check. Update this file in the same change.

## Filing a task as an issue

Title: the task heading (`T4 — Loopback send and receive`).

Body: the task section, including the test names. One task per issue keeps the review small.

## Suggested order

| Task | You end up able to |
| --- | --- |
| [T0](#t0--build-and-read-the-tree) | Build, run the scaffold tests, find a stub |
| [T1](#t1--name-every-error) | Turn `Error` into a stable string |
| [T2](#t2--validate-a-classical-can-frame) | Accept or reject a frame with no bus involved |
| [T3](#t3--open-and-close-the-loopback-bus) | Honor `is_open()` |
| [T4](#t4--loopback-send-and-receive) | Move a frame through an in-memory queue |
| [T5](#t5--fault-injection) | Drop, corrupt, delay, or fail a transmit |
| [T6](#t6--test-runner-checks) | Record a pass or a failure for one frame |
| [T7](#t7--socketcan-and-vcan) | Do the same send/receive on `vcan0` |

T5 has two layers: the injector by itself, then the loopback bus calling it. Do the injector tests before the loopback fault tests. Do the delay fault after drop, corrupt, and error. T7 is last because it needs the kernel.

## T0 — build and read the tree

No code changes.

```bash
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug --output-on-failure
./build/gcc-debug/aerotest --version
```

Read `include/aerotest/bus.hpp`, then `src/loopback_bus.cpp`, then one disabled test in `tests/loopback_bus_test.cpp`. The comments on `Bus` are the contract. The test is a worked example of that contract.

**What you'll learn:** a CMake preset is a named configure/build/test triple, and a test can compile against a function that still returns `NotImplemented`.

**Done when:** the preset run passes, and you can point at one `TODO(step-1)` and the test that will go green when you fill it in.

## T1 — name every error

`to_string` is the smallest function in the library and the place to practice an exhaustive `switch`.

**Files:** `include/aerotest/status.hpp`, `src/status.cpp`

**Tests:** `tests/status_test.cpp` — remove the prefix on `DISABLED_ToStringSpellsEveryError`.

The spellings the test requires:

| Enumerator | Spelling |
| --- | --- |
| `Ok` | `ok` |
| `NotOpen` | `not_open` |
| `Timeout` | `timeout` |
| `InvalidArgument` | `invalid_argument` |
| `Io` | `io` |
| `FaultInjected` | `fault_injected` |
| `NotImplemented` | `not_implemented` |

Handle every enumerator. Leave out `default`. A `default` that returns `"unknown"` hides a new enumerator from `-Wswitch`, which is already part of `-Wall`. With a `default`, this test can still pass while the compiler stays quiet. Turn on `-DAEROTEST_WERROR=ON` for this task if you want a missing case to fail the build.

```cpp
const char* to_string(Error error) {
    switch (error) {
    case Error::Ok:
        return "ok";
        // one returning case per enumerator, no default
    }
}
```

**What you'll learn:** `enum class` does not convert to `int` on its own, and a `switch` with no `default` is how you notice a new enumerator.

**Done when:** `Status.ToStringSpellsEveryError` passes, and adding a fake enumerator makes the file fail to compile cleanly under `-Werror` (then delete the fake).

## T2 — validate a classical CAN frame

`Message::validate` is a pure function. No sockets, no queue.

**Files:** `include/aerotest/message.hpp` (the rules are on the method), `src/message.cpp`

**Tests:** in `tests/message_test.cpp`, remove the prefix on:

- `DISABLED_AcceptsMaxStandardFrame`
- `DISABLED_AcceptsExtendedPast11Bits`
- `DISABLED_AcceptsMaxExtendedId`
- `DISABLED_AcceptsRemoteRequestWithoutData`
- `DISABLED_RejectsStandardIdPast11Bits`
- `DISABLED_RejectsExtendedIdPast29Bits`
- `DISABLED_RejectsNinthDataByte`
- `DISABLED_RejectsRemoteRequestWithData`

Rules, also written on the method:

- A standard id is in `0 .. 0x7FF`. `0x800` is illegal for `IdKind::Standard` and legal for `IdKind::Extended`.
- An extended id is in `0 .. 0x1FFFFFFF`.
- `data.size()` is at most 8. This step is classical CAN. A 9th byte is `InvalidArgument`, not CAN FD.
- `remote_request` requires an empty `data` vector.
- Id 0 and an empty data vector are legal.
- Success is `Status::success()`. A broken rule is `Status::failure(Error::InvalidArgument, ...)`. The tests check `code`, not the wording of `detail`.

`DefaultsAreEmpty` and `AssigningFieldsRoundTrips` are enabled. They only store fields. Leave them passing.

**What you'll learn:** a protocol rule as a function you can test without a device, and `Status` as the way a function says no.

**Done when:** those eight tests pass.

## T3 — open and close the loopback bus

`is_open()`, `name()`, and `queued()` already read the members. `open()` and `close()` are the stubs.

**Files:** `include/aerotest/loopback_bus.hpp`, `src/loopback_bus.cpp`

**Tests:**

- `DISABLED_OpenThenIsOpen`
- `DISABLED_SecondOpenKeepsQueuedFrames` (finish this one after T4's `send` works; `open` itself is done when the first test passes and a second `open()` returns `Ok` without clearing `queue_`)
- `DISABLED_CloseWhenClosedIsOk`
- `DISABLED_CloseDropsQueuedFrames` (also needs `send`)

Behavior:

- `open()` sets `open_` and returns `Ok`. Calling it again stays open and keeps whatever is already queued.
- `close()` clears `queue_`, clears `open_`, and returns `Ok`. Calling it on a closed bus is still `Ok`.

**What you'll learn:** a derived class filling in an interface, and an invariant you can say out loud (`is_open()` matches the flag you just set).

**Done when:** `OpenThenIsOpen` and `CloseWhenClosedIsOk` pass. The other two pass once T4 can queue a frame.

## T4 — loopback send and receive

Still no threads and no sleeping. `receive` looks at the queue once and returns.

**Files:** `src/loopback_bus.cpp`. The private `Queued` struct is already on the bus: a frame plus the `steady_clock` time at which `receive` may pop it. A time in the past, including the clock's epoch, means "ready now".

**Tests:**

- `DISABLED_SendBeforeOpen`
- `DISABLED_ReceiveBeforeOpen`
- `DISABLED_NegativeTimeoutIsInvalid`
- `DISABLED_RejectsInvalidFrame`
- `DISABLED_RoundTrip`
- `DISABLED_FifoOrder`
- `DISABLED_ReceiveOnEmptyTimesOut`
- `DISABLED_TimeoutLeavesOutputUntouched`
- `DISABLED_StampsTimestamp`
- `DISABLED_SecondOpenKeepsQueuedFrames`
- `DISABLED_CloseDropsQueuedFrames`

`send`:

1. If the bus is closed, return `NotOpen`. Do not queue.
2. Copy `message`. The caller's object stays as they passed it.
3. `validate()` the copy. On failure, return that status and do not queue.
4. With no fault injector, push the copy with `ready_at` set to `steady_clock::now()`.
5. Return `Ok`. The fault-injector branch is T5; leave a clear place for it.

`receive`:

1. If the bus is closed, return `NotOpen` and do not write `out`. A negative timeout does not override this.
2. If `timeout` is negative, return `InvalidArgument`. Do not pop.
3. If the queue is empty, or `steady_clock::now() < ready_at` for the front frame, return `Timeout` and do not write `out`. Pop when `now() >= ready_at`. Do not sleep, even when `timeout` is 50ms. The parameter exists so `SocketCanBus` can block. The loopback has nothing else to wait for.
4. Otherwise pop the front frame, set `timestamp` from `steady_clock::now().time_since_epoch()`, write `out`, and return `Ok`.

Two frames sent in order come back in that order. `StampsTimestamp` compares against `steady_clock`, so stamp from that clock.

**What you'll learn:** a queue as a stand-in for hardware, copying before a later stage mutates the frame, and a timeout argument you deliberately do not block on.

**Done when:** the tests in the list pass. `ReceiveOnEmptyTimesOut` uses a 50ms timeout and should still return immediately. If that test takes about 50ms, `receive` is sleeping.

## T5 — fault injection

The injector decides. The bus obeys. `clear()` and `pending()` already work. `arm` and `apply_tx` do not.

**Files:** `include/aerotest/fault_injector.hpp`, `src/fault_injector.cpp`, then the `send` path in `src/loopback_bus.cpp`.

**Injector tests** in `tests/fault_injector_test.cpp`:

- `DISABLED_ArmIncreasesPending`
- `DISABLED_RejectsZeroRemaining`
- `DISABLED_RejectsRemainingBelowMinusOne`
- `DISABLED_EmptyInjectorPassesThrough`
- `DISABLED_DropSuppressesDelivery`
- `DISABLED_CorruptFlipsTheByte`
- `DISABLED_CorruptRejectsABadIndex`
- `DISABLED_ErrorReportsFaultInjected`
- `DISABLED_DelaySetsExtraDelay`
- `DISABLED_OtherIdsDoNotMatch`
- `DISABLED_RemainingCountsDown`
- `DISABLED_NegativeOneDoesNotExpire`
- `DISABLED_FirstArmedFaultWins`
- `DISABLED_ClearForgetsArmedFaults`

**Loopback tests** once `send` calls the injector:

- `DISABLED_DropFaultIsNotDelivered`
- `DISABLED_CorruptFaultChangesOnlyTheCopy`
- `DISABLED_ErrorFaultFailsTheSend`
- `DISABLED_DelayFaultWaitsForTheClock`

`arm`:

- `remaining == 0` or `remaining < -1` returns `InvalidArgument` and stores nothing.
- Otherwise append the spec. `pending()` is the number of stored specs.

`apply_tx` edits the `Message&` it is given. The unit tests pass their own message and expect a corrupt fault to change it. The bus must pass a copy, which is why `CorruptFaultChangesOnlyTheCopy` checks that the caller's bytes survived.

With nothing armed, or after `clear()`: `code` is `Ok`, `deliver` is true, `extra_delay` is zero, and the message is unchanged.

Otherwise walk the armed specs in the order they were armed. The first spec whose `match_id` is empty or equal to `message.id` is the one that runs. Later specs wait. A spec that does not match stays armed and does not count down.

| Kind | `code` | `deliver` | Message | `extra_delay` |
| --- | --- | --- | --- | --- |
| `Drop` | `Ok` | false | unchanged | 0 |
| `Corrupt` | `Ok` | true | `data[byte_index] ^= xor_mask` | 0 |
| `Corrupt` with `byte_index >= data.size()` | `InvalidArgument` | false | unchanged | 0 |
| `Error` | `FaultInjected` | false | unchanged | 0 |
| `Delay` | `Ok` | true | unchanged | `spec.delay` |

A corrupt spec that cannot be applied stays armed (`CorruptRejectsABadIndex`). Every other match consumes one use: if `remaining` was greater than 0, decrement it and drop the spec at 0. `remaining == -1` does not decrement.

In `LoopbackBus::send`, after a successful `validate`:

- No injector: queue the copy at `ready_at = now`, as in T4.
- Injector set: call `apply_tx` on the copy.
  - If `code` is not `Ok`, return that code and do not queue.
  - If `deliver` is false, return `Ok` and do not queue. A drop looks like a successful send that nobody will receive.
  - Otherwise queue the copy at `ready_at = now + extra_delay`.

`send` does not sleep. `DelayFaultWaitsForTheClock` sleeps in the test (50ms delay, then 80ms in the test, then `receive`). An immediate `receive` returns `Timeout` and leaves the frame queued.

**What you'll learn:** a policy object the bus calls, a filter that does not count as a use, and the difference between "the injector's caller handed over the real object" and "the bus handed over a copy".

**Done when:** the injector tests pass, then the four loopback fault tests pass.

## T6 — test runner checks

The runner does not open or close the bus. The caller does. `Report::passed` and `failure_count` already walk `checks`. Every call to `expect_message` or `expect_echo` appends one `CheckResult` before returning it, including failures. The stub returns a failure and records nothing, so the tests that expect a recorded check fail today.

**Files:** `include/aerotest/test_runner.hpp`, `src/test_runner.cpp`

**Tests** in `tests/test_runner_test.cpp`:

- `DISABLED_EchoMatchesOnLoopback`
- `DISABLED_ClosedBusIsARecordedFailure`
- `DISABLED_TimeoutIsARecordedFailure`
- `DISABLED_MismatchIsARecordedFailure` (this one also needs T5's corrupt fault)

Compare `id`, `id_kind`, `remote_request`, and `data`. Do not compare `timestamp`. The outbound frame's timestamp stays 0 and the loopback stamps the received copy, so `EchoMatchesOnLoopback` fails if you compare that field.

Leave `requirement_id` empty. Step 2's scenario runner copies a requirement id onto the check it stores. These tests do not set one.

`expect_echo`:

- If the bus is closed, record a failure, do not send, and leave the bus closed.
- Otherwise send a copy of `outbound`, then do the same comparison as `expect_message`.
- A send or receive error is a failed check with a non-empty `detail`. The tests do not lock the wording. They lock the name, `passed`, and that the report grew by one.

`expect_message` only receives. `TimeoutIsARecordedFailure` opens the bus, sends nothing, and expects a recorded failure.

`MismatchIsARecordedFailure` runs a clean echo, arms a one-byte corrupt fault, and expects the next echo to fail while the caller's payload is unchanged. Do it last.

**What you'll learn:** a thin check that records every attempt, and which fields make two frames "the same frame" for a test.

**Done when:** the four tests pass. `FreshReportIsAPass` and `ReportCountsFailures` were already enabled; they still pass.

## T7 — SocketCAN and vCAN

Linux only. `name()` and `is_open()` are done: the bus is open when `socket_fd_ >= 0`. There is no second flag.

**Files:** `include/aerotest/socketcan_bus.hpp`, `src/socketcan_bus.cpp`

**Tests with no interface** in `tests/socketcan_bus_test.cpp`:

- `DISABLED_SendBeforeOpen`
- `DISABLED_CloseWhenClosedIsOk`
- `DISABLED_RejectsEmptyName`

**Tests that need `vcan0`** in `tests/vcan_bus_test.cpp`. They are a separate binary labeled `needs-vcan`. Presets exclude that label, so CI stays green after you remove `DISABLED_`. Run them yourself:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
# remove DISABLED_ on the three tests, rebuild, then:
ctest --test-dir build/gcc-debug -L needs-vcan --output-on-failure
```

- `DISABLED_VcanRoundTrip`
- `DISABLED_VcanExtendedRoundTrip`
- `DISABLED_VcanRemoteFrame`

`open`:

- An empty interface name returns `InvalidArgument` and leaves the fd at `-1`. No system call yet.
- Otherwise create a raw CAN socket, look up the interface index, and bind. The headers you will include are `<linux/can.h>`, `<linux/can/raw.h>`, `<net/if.h>`, `<sys/ioctl.h>`, `<sys/socket.h>`, `<poll.h>`, and `<unistd.h>`.
- Set `CAN_RAW_RECV_OWN_MSGS`. A raw socket does not hear its own frames until you ask, and the vCAN tests use one socket for both directions. This is the option that makes `VcanRoundTrip` able to pass.
- On any failure, close what you opened, leave `socket_fd_` at `-1`, and return `Io` with `detail` set from `errno`.
- Store the live descriptor in `socket_fd_`.

`close` and the destructor:

- If the fd is `-1`, `close()` returns `Ok`.
- Otherwise close it, set `socket_fd_` back to `-1`, and return `Ok`. The destructor does the same close. `VcanRoundTrip` closes and opens again, so a leftover fd fails the second open or leaks the first one.

`send`:

- Closed bus: `NotOpen`.
- `validate()` the frame. On failure, `InvalidArgument`, and do not write. The vCAN tests send legal frames; this keeps the bus consistent with the loopback.
- Pack a `can_frame`. Standard ids use 11 bits and do not set `CAN_EFF_FLAG`. Extended ids set `CAN_EFF_FLAG`. `remote_request` sets `CAN_RTR_FLAG` and a data length of 0. `can_dlc` is the data length, from 0 to 8. Copy the data bytes. Then `write` the frame.
- Step 1 does not apply the fault injector on this bus. The loopback is where faults are tested. The slot still works; `FaultSlotRoundTrips` already checks it.

`receive`:

- Closed bus: `NotOpen`, and do not write `out`.
- Negative timeout: `InvalidArgument`.
- `poll` for `timeout`. A `0` return is `Timeout`. Retry `poll` when it fails with `EINTR`. Any other `-1` is `Io`.
- `read` one `can_frame`. Rebuild `id` with `CAN_SFF_MASK` or `CAN_EFF_MASK` so the flags are not part of the id. Set `id_kind` from `CAN_EFF_FLAG` and `remote_request` from `CAN_RTR_FLAG`. Copy `can_dlc` data bytes.
- Stamp `timestamp` from `steady_clock`, same as the loopback.

`VcanRoundTrip` sends one standard frame, reads it back, checks that a second `receive` times out, then closes and opens again. The extended test uses id `0x18FF50E5`. The remote test sends a remote frame and expects empty data on the way back.

CANServer's fast path was this socket, with `cansend` as a fallback. AeroTest has no fallback. One path is the thing the tests can pin down.

**What you'll learn:** a file descriptor as the open/closed bit, `poll` as a timeout, how a `can_frame` carries the id flags, and why a socket is silent about its own transmit until `CAN_RAW_RECV_OWN_MSGS` is set.

**Done when:** the three hardware-free tests pass under `ctest --preset gcc-debug`, and the three `needs-vcan` tests pass on a machine where `vcan0` is up.

## Done with step 1

- T1 through T6 tests pass with the `DISABLED_` prefix removed.
- T7's hardware-free tests pass the same way.
- The three vCAN tests pass when you opt in with `-L needs-vcan`.
- `ctest --preset gcc-debug` and `ctest --preset clang-debug` still pass. The vCAN label stays excluded, so a laptop without the kernel module is not a CI failure.
- No new dependency. `yaml-cpp` and `spdlog` wait for later steps.

`gcc-asan` is worth one run after T4 and T5. It builds with:

```bash
cmake --preset gcc-asan
cmake --build --preset gcc-asan
ctest --preset gcc-asan --output-on-failure
```
