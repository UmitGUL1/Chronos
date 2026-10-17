# Implementation status — 2026-10-17

## Development preview: Windows/Linux CI passes

[Verification evidence](docs/ci-verification.md) records successful run
[39014818010](https://github.com/UmitGUL1/Chronos/actions/runs/39014818010)
on source revision b43a10f30f590b13d58a9857254ddcd4857b50eb.

- Windows and Ubuntu core Debug: **44 passed per platform**.
- Windows and Ubuntu Qt 6.8.3 Debug: **45 passed per platform**.
- Ubuntu AddressSanitizer + UndefinedBehaviorSanitizer: **44 passed**.
- 42 GoogleTest cases + CLI help/reference; desktop adds one offscreen smoke.
- All 16 JSON scenarios replay deterministically and conserve CPU/job service
  in the scenario integration test.

Local compiler installation failed earlier; source work continued under explicit
user authorization. The project was subsequently uploaded to the user-created
UmitGUL1/Chronos repository and real CI exposed/fixed compilation and QML errors.
No local test execution is claimed.

| Phase | Implementation and verification scope |
| --- | --- |
| 0 | C++20/CMake configure/build succeeds on GCC/MSVC; Qt kit resolves in CI |
| 1 | Time, jobs, events, recording, CLI: reference/invariant tests pass |
| 2 | Seven policies: reference scheduling tests pass |
| 3 | Mutexes, inversion and transitive inheritance: reference tests pass |
| 4 | Cycles, nested IRQs/masks and virtual watchdog: tests pass |
| 5 | Metrics and assumption-limited RMS/EDF analysis: reference tests pass |
| 6 | Qt/QML builds and offscreen startup/worker smoke passes; full interaction pending |
| 7 | 16 scenarios, replay, comparison and report persistence tests pass |
| 8 | Worker/independent-engine tests pass; ASan/UBSan passes; race/stress/profile work pending |
| 9 | Docs/license, Windows runtime ZIP and actual screenshots ready; full manual QA pending |

## CI fixes

Added the missing model namespace closure, removed an invalid QML Flow
implicitHeight assignment, allowed automatic Qt architecture selection and
enabled Windows console diagnostics for CTest. These fixes are included in the
successful source revision above.

## Limits and incomplete work

Passing tests establish the covered references and bounded invariants, not a
universal correctness guarantee or production RTOS certification. Manual
interaction/visual/resizing checks, ThreadSanitizer, host stress, immutable
dependency/action hash locks and signed installer remain pending. A Windows
runtime ZIP and three actual Release benchmark samples are now available; see
docs/ci-verification.md and docs/performance.md. Resource/interrupt configuration
uses the full JSON editor.

Scheduler/wait/donation operations scan jobs and can be quadratic. Snapshot
serialization copies history before UI clipping. Cancellation is at event-batch
boundaries and cannot interrupt a large batch or synchronous export.

Multi-core scheduling, generalized recovery, job restart/per-job watchdogs and
fixed-priority response-time recurrence are not implemented. Isolated scheduler
latency and peak-memory instrumentation are not implemented. No coverage
percentage or visual QA claim is made. The runtime ZIP is a development preview.

## Next milestone

Run manual desktop interaction/resize/shutdown checks and prepare a signed
installer if distribution beyond the preview is needed.
