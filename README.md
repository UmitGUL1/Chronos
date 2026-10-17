# ChronOS

**Real-Time Operating System & Scheduling Simulator** — C++20, CMake, Qt Quick.

ChronOS models job releases, scheduling, resource blocking, priority inversion,
interrupts and deadlines using deterministic integer virtual time. It is an
engineering/learning simulator, **not a functioning production RTOS kernel**.

> Development preview with passing GitHub Actions builds on Windows and Linux.
> Core configurations pass 44 CTest entries each; Qt 6.8.3 configurations pass
> 45 each, including an offscreen desktop launch. Linux ASan/UBSan passes 44.
> See [CI verification](docs/ci-verification.md) for the exact source revision
> and run. Full manual GUI testing and release packaging remain pending.

## Source capabilities

- Separate Task definitions and periodic Job instances, including overlap.
- FCFS, SJF, SRTF, Round Robin, fixed priority, RMS and EDF strategies.
- FIFO exclusive mutexes, blocking, nested/transitive priority inheritance.
- Wait-for graph cycle detection with historical deadlock reports.
- Nested/prioritized interrupts, pending handlers, masks and latency.
- Virtual subsystem watchdog with heartbeats, alerts and optional termination.
- Service/wait/blocking metrics and guarded RMS/EDF analytical checks.
- JSON scenario load/save, deterministic replay and policy comparisons.
- CLI plus Qt/QML dashboard with a worker thread and value snapshots.
- Timeline zoom, pan, hover, selection, deadline/preemption/blocking indicators.
- 16 configurable scenarios; JSON/CSV/SVG reports; benchmark workload source.
- 42 GoogleTest cases, CLI integration checks and Windows/Linux CI workflows.

There are no fabricated screenshots, pass counts, CI badges or performance claims.
See [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) for incomplete work.

## Build the CLI and tests

Requires CMake 3.24+, a C++20 compiler, Git and initial network access.
Dependencies are fetched at GoogleTest v1.15.2 and nlohmann/json v3.11.3 release
tags. There is no vcpkg requirement; FetchContent is the selected dependency route.

On Windows, install Visual Studio 2022 Build Tools with Desktop development with
C++, Windows SDK and CMake. Open Developer PowerShell, then from this directory:

```powershell
cmake --preset core-debug
cmake --build --preset core-debug
ctest --preset core-debug
.\build\core-debug\Debug\chronos-cli.exe --scenario scenarios/basic-fcfs.json
```

On Linux, install CMake, Git and a C++20 compiler. The same presets apply; a
single-configuration generator puts executables in `build/core-debug/`, without
the `Debug/` subdirectory. Windows MSVC and Ubuntu GCC builds are covered by CI.

```sh
./build/core-debug/chronos-cli --scenario scenarios/priority-inheritance.json
./build/core-debug/chronos-cli --scenario scenarios/sjf-srtf.json --scheduler sjf
./build/core-debug/chronos-cli --scenario scenarios/deadlock.json --export results/new-run
./build/core-debug/chronos-cli --scenario scenarios/edf.json --compare > comparison.json
```

`--headless` explicitly selects the same CLI execution mode. Exit status 0 means
a run completed to its horizon, even if simulated jobs missed deadlines or
deadlocked; inspect the report. Invalid input returns 2; runtime/I/O failure 3.
Reports require a fresh directory and refuse to overwrite existing report files.

## Build the desktop

### Portable Windows build

The [Windows portable package workflow](https://github.com/UmitGUL1/Chronos/actions/workflows/windows-package.yml)
builds Release, runs the full suite, deploys Qt/QML and the MSVC runtime, then
checks the deployed CLI and desktop with SDK/compiler paths removed. A successful
run provides a **ChronOS-Windows-x64** ZIP artifact. Extract the complete archive
and open **Start-ChronOS.cmd**. No Qt SDK or compiler should be needed to run it.
Artifacts expire after 30 days; rerun the workflow to produce a new build.
See [package instructions](docs/windows-package.md) and the
[manual acceptance checklist](docs/manual-qa.md). The ZIP is an unsigned development
build, not an installer or a signed production release.

### Build from source

Install a compiler-compatible Qt 6.5+ kit with Qt Quick, Quick Controls 2 and Qt
Quick Dialogs. This includes MSVC vs MinGW ABI compatibility on Windows.

```sh
cmake --preset desktop-debug -DCMAKE_PREFIX_PATH=/path/to/Qt/kit
cmake --build --preset desktop-debug
ctest --preset desktop-debug
```

Run `chronos-desktop` from the repository root to discover `scenarios/`, or use
the Load dialog. CMake install also copies scenarios to share/chronos/scenarios.
Nine pages expose dashboard, scheduler lab, task manager, resources, interrupts,
deadlock graph, analytics/comparison, JSON scenario library and model settings.
All scenario fields remain editable in JSON. Task or policy edits reset the run.
Stop pauses; Reset clears runtime history. The first event step processes time
zero; +1 tick uses the same engine and advances one virtual tick.

The offscreen smoke test launches QML, steps the engine, starts/pauses playback
and rejects QML warnings. Full manual interaction and visual/resizing inspection
remain pending. No screenshots were captured. Qt runtime deployment packaging
(macdeployqt) is not implemented. Windows runtime ZIP generation is configured
in its own workflow and validated separately from the SDK-based Debug smoke.

## Demonstrations

| Scenario | Expected result from manual reference reasoning |
| --- | --- |
| basic-fcfs | A 0–3, B 3–5; utilization 5/8 |
| fixed-preemption | A 0–2, B 2–4, A 4–7 |
| priority-inversion | H completes at 12; M delays the lock owner |
| priority-inheritance | H completes at 8; L inherits H's priority |
| deadlock | A/B circular wait detected at time 4 |
| interrupt-storm | All service spent in handlers; worker misses deadline |
| watchdog | Heartbeats at 2/4; timeout alerts at 9/14 |

These are manual reference expectations. Exact FCFS, fixed-priority, inversion,
inheritance, deadlock and watchdog outcomes are asserted by passing tests.
Every bundled scenario also passes replay and accounting integration checks.

## Engineering documentation

- [Architecture](docs/architecture.md), [diagrams](docs/diagrams.md),
  [design decisions](docs/design-decisions.md)
- [Algorithms](docs/algorithms.md), [time model](docs/mathematical-model.md),
  [theory](docs/scheduling-theory.md), [metrics](docs/metrics.md)
- [Resources/interrupts](docs/resources-and-interrupts.md),
  [host concurrency](docs/concurrency.md)
- [Testing](docs/testing.md), [performance methodology](docs/performance.md),
  [interview guide](docs/interview-guide.md)
- [Engineering review](docs/engineering-review.md), [roadmap](ROADMAP.md)

RMS uses a sufficient bound; exceeding it is inconclusive. EDF's utilization
test is limited to the ideal implicit-deadline single-CPU model. Multi-core,
context-switch cost, general recovery, isolated latency measurement and peak
memory measurement are not implemented. See the documented limits before using
results for reasoning about real systems.

## Verification and CI

```sh
node scripts/check-assets.mjs
cmake -S . -B build/asan -DCHRONOS_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/asan
ctest --test-dir build/asan --output-on-failure
```

Local compilation remains unavailable, but GitHub Actions has compiled and
tested the project on Windows and Linux, including Qt smoke and ASan/UBSan.
The source repository is [UmitGUL1/Chronos](https://github.com/UmitGUL1/Chronos).
CI runs do not replace manual desktop interaction or performance measurements.

License: MIT for project code. Dependencies retain their own licenses.
