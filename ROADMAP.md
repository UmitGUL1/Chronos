# Roadmap

Verification is scoped to actual tests. See docs/ci-verification.md for the
successful Windows/Linux matrix and exact source revision.

| Work | Status |
| --- | --- |
| C++20/CMake core and desktop | Windows/Linux CI builds verified |
| Deterministic engine and overlapping jobs | Reference/replay/accounting tests pass |
| Seven schedulers | Reference tests pass |
| Mutexes, inversion, transitive inheritance | Reference tests pass |
| Deadlock history, nested IRQs/masks, watchdog | Reference tests pass |
| Metrics and assumption-limited RMS/EDF | Reference tests pass |
| CLI, JSON configuration, report export | Persistence/reference tests pass |
| Qt/QML desktop and worker bridge | Offscreen startup/step/play/pause smoke passes |
| Full interactive and visual desktop QA | Pending |
| 16 scenario assets | Replay and accounting integration passes |
| Linux ASan/UBSan | All 44 CTest entries pass |
| Host stress and ThreadSanitizer | Planned |
| 10-10,000-job benchmark runner | Three Windows Release samples recorded |
| Isolated scheduling latency/peak memory instrumentation | Planned |
| Dependency/action commit hash locking | Planned |
| Windows desktop runtime ZIP | Release tests and deployed startup pass |
| Actual screenshots | Pending manual desktop QA |
| Multi-core scheduling/general recovery | Later extension |
