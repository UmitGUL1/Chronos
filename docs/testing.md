# Verification status and commands

**C++ tests have not run.** The repository declares 42 GoogleTest cases, plus CLI
help, CLI reference and optional desktop smoke CTest entries. The count is from
source inspection, not an executed/pass count. No coverage percentage is claimed.

```sh
cmake --preset core-debug
cmake --build --preset core-debug
ctest --preset core-debug
node scripts/check-assets.mjs
```

Core references cover FCFS service/idle time and manual metrics, SJF and SRTF,
RR quantum and simultaneous arrivals, fixed priorities, RMS and EDF, periodic
overlap, completion exactly at deadlines, nonterminating misses, PI inversion,
transitive donation, invalid lock use, two/three-job deadlock, interrupt nesting,
non-nesting, masks, deadline interference, heartbeat generations, timeout cleanup,
analysis assumptions, configuration errors, replay, export contents and no-overwrite.

Scenario integration runs every bundled JSON twice and checks deterministic
complete reports, service conservation, monotonic contiguous intervals and job
budget invariants. Host tests compare independent concurrent engines and verify
pre-requested cancellation does not advance time.

With Qt installed:

```sh
cmake --preset desktop-debug -DCMAKE_PREFIX_PATH=/path/to/Qt/kit
cmake --build --preset desktop-debug
ctest --preset desktop-debug
```

The optional offscreen smoke launches QML, steps the engine, starts/pauses playback
and rejects QML warnings, empty report or controller errors. It is not a complete
GUI interaction suite. Manually verify all nine pages, resizing, task editing,
policy constraints, JSON load/save errors, reset, pause, timeline pan/zoom/hover,
exports, comparison, shutdown during playback and shutdown during comparison.

CI defines Windows/Linux core and desktop builds, plus Linux ASan/UBSan. It has
not been run. ThreadSanitizer, clang-tidy and format enforcement are not CI gates
yet. The assets script parses JSON and verifies CMake file references only; it
does not prove C++ syntax, scheduling correctness or QML runtime behavior.
