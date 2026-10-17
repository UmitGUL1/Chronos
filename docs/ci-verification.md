# CI verification — 17 October 2026

Verified source revision: `b43a10f30f590b13d58a9857254ddcd4857b50eb`.

[Successful GitHub Actions run 39014818010](https://github.com/UmitGUL1/Chronos/actions/runs/39014818010)
completed with every job successful.

| Configuration | Compiler / Qt | CTest outcome | Test execution time |
| --- | --- | --- | --- |
| Ubuntu core, Debug | GCC 13.3.0 | 44 passed, 0 failed | 0.15 s |
| Windows core, Debug | MSVC 19.51.36260.0 | 44 passed, 0 failed | 0.66 s |
| Ubuntu desktop, Debug | GCC 13.3.0 / Qt 6.8.3 | 45 passed, 0 failed | 2.13 s |
| Windows desktop, Debug | MSVC 19.51.36260.0 / Qt 6.8.3 | 45 passed, 0 failed | 3.04 s |
| Ubuntu ASan + UBSan, Debug | GCC 13.3.0 | 44 passed, 0 failed | 1.34 s |

Times are CTest's recorded aggregate execution durations for this run. They are
not simulator performance benchmarks or cross-platform speed comparisons.

The suite contains 42 GoogleTest cases and two CLI integration entries. The Qt
configuration adds one offscreen smoke entry. Repeating the same tests across
configurations does not create additional distinct cases. Scenario integration
loads all 16 bundled scenarios, replays each and checks deterministic reports,
CPU time conservation and per-job budget conservation.

The Qt smoke loads the actual QML module, steps one tick and one event batch,
starts/pauses the worker and fails on QML warnings, empty reports or controller
errors. It is not a full mouse/keyboard interaction or visual layout test.

## Issues found and fixed through CI

1. Missing namespace closure in src/core/model.cpp prevented compilation.
2. Explicit gcc_64 Qt architecture selection did not match the installer package
   metadata. Platform selection now uses aqtinstall's automatic resolution.
3. QML Flow implicitHeight is read-only; assigning it prevented desktop startup.
4. The Windows desktop target now retains console diagnostics for CTest.

## Remaining verification

Manual task editing, file dialogs, timeline interactions, full page interaction,
resizing/visual inspection still need checks.
ThreadSanitizer and host stress testing are not implied by ASan/UBSan success.
Isolated scheduling latency/peak-memory measurements remain pending.

## Portable Windows follow-up

Source `d5edcb351a40772050b47d44a2a7bf754eaf4e0d` passed all five Debug matrix
jobs in [run 39016039205](https://github.com/UmitGUL1/Chronos/actions/runs/39016039205).
The [Release package run 39016039228](https://github.com/UmitGUL1/Chronos/actions/runs/39016039228)
passed 45 tests in 2.30 seconds, then verified the deployed priority-inheritance
CLI result (H completes at tick 8) and offscreen GUI smoke with SDK/compiler
paths removed. The offscreen plugin is explicitly bundled after deployment
testing exposed its absence in the first package attempt.

[Artifact 12094821269](https://github.com/UmitGUL1/Chronos/actions/runs/39016039228/artifacts/12094821269)
contains the runtime, launcher, scenarios, license notices, verification.json,
file checksums, manual QA checklist and measurement files. The downloaded ZIP
was verified at 37,771,444 bytes with SHA-256:
`83492e02f6067ed80e89e17aa0f4dbc1736a5b3856a110b35fae92073aa17b73`.
Artifact retention is 30 days. This is an unsigned development preview.

Three actual Windows Release benchmark runs are recorded with raw CSV and
environment metadata; see performance.md for results and workload limitations.
