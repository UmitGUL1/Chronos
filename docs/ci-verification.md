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
resizing/visual inspection and packaged Windows distribution still need checks.
ThreadSanitizer and host stress testing are not implied by ASan/UBSan success.
Benchmark execution and isolated scheduling latency/peak-memory measurements
remain pending; no performance claim is made.
