# Roadmap

Status legend: Source implemented means written, **not verified**. Verified is
reserved for successful build/runtime evidence, which is currently absent.

| Work | Status |
| --- | --- |
| C++20/CMake targets and presets | Source implemented; presets parsed |
| Deterministic engine and overlapping jobs | Source implemented, unverified |
| Seven schedulers | Source implemented, unverified |
| Mutexes, inversion, transitive inheritance | Source implemented, unverified |
| Deadlock graph/cycle history | Source implemented, unverified |
| Nested IRQs, masks, latency | Source implemented, unverified |
| Watchdog heartbeat/alert/termination | Source implemented, unverified |
| Metrics and assumption-limited RMS/EDF | Source implemented, unverified |
| CLI, JSON configuration, report export | Source implemented, unverified |
| Qt/QML desktop and worker bridge | Source implemented, unverified |
| 16 scenario assets | JSON syntax verified; simulation behavior unverified |
| GoogleTest references and integration | 42 cases declared, unexecuted |
| Host stress and race validation | Planned / requires toolchain |
| 10–10,000-job benchmark runner | Source implemented; no measured results |
| Dedicated scheduling latency/peak memory instrumentation | Planned |
| Windows/Linux/Qt/sanitizer CI | Workflow written, not run |
| Dependency/action commit hash locking | Planned |
| Actual screenshots and desktop runtime package | Planned |
| Multi-core scheduling and general recovery | Later extension |

The next milestone is a passing CI matrix followed by real desktop interaction
checks, not another claim of feature completion based solely on source files.
