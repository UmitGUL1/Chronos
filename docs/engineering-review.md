# Engineering review — source-only assessment

## Outcome

A substantial C++20/Qt source implementation now exists. It is an **unverified
source preview**, not a completed tested release. No executable was produced.
User authorization allowed proceeding after the original local build gate failed.

## What was reviewed

- Ownership: engine values and unique_ptr policy; worker held by joined jthread.
- Timing: bounded integer ticks; stable heap ordering; CPU-offset actions;
  timestamp batches and exact-deadline completion rule.
- Policies: distinct strategy classes; preemption/quantum owned by engine;
  deterministic ready sequence; RMS uses fixed dispatcher after period ranking.
- Synchronization: exclusive ownership, FIFO handoff, cleanup, fixed-point donation.
- Diagnostics: chain-cycle detection, historical reports, no implicit recovery.
- Interrupts: strict-priority nesting, pending queue, masks, resumed service.
- Watchdog: virtual generation invalidation, alert vs explicit termination policy.
- Analytics: per-population metrics, guarded analytical assumptions, no fake data.
- GUI: queued commands and value snapshots; full scenario access through JSON.
- Inputs/outputs: bounded configuration, schema checks, fresh report directories,
  escaped CSV/SVG labels and no credentials/personal data in project sources.

## Source review fixes

RR expiration originally omitted the preemption counter when a different ready
job displaced unfinished work; a conditional count/event was added. UI snapshot
coalescing originally risked dropping the final paused update; a refresh flag
and condition-variable wake-up were added. Their runtime behavior is not verified.
Policy selection was split into actual concrete strategy implementations, with
RMS explicitly reusing fixed-priority selection rather than duplicating it.
Maximum completed-job real-time response was separated from first-dispatch delay.

## Actual evidence

The Node asset check succeeded: 16 JSON scenarios parse, all 17 CMake-referenced
source inputs exist, and 42 GoogleTest cases are declared. Portable CMake can
list the two configure presets. This is not compilation, CTest, QML validation,
coverage evidence, a race check or a performance measurement.

No C++ test pass/fail count, benchmark number, hardware-performance claim or
screenshot exists. CI is prepared but unexecuted. See testing.md for exact
commands and required manual GUI checks.

## Material risks and unfinished requirements

Build and runtime errors can remain. GUI polish/layout is written without visual
inspection; several engineering details use formatted inspection panels rather
than fully specialized widgets. Complete interaction automation, screenshots,
Qt runtime deployment, memory profiling, isolated scheduling latency, dependency
commit hashes and ThreadSanitizer remain future work. Performance can be quadratic
and snapshot generation copies large histories. Export is synchronous in the
worker; cancellation cannot interrupt one large timestamp batch or an export.

The code models one ideal CPU and zero overhead. It does not implement a
production RTOS, multicore correctness, general deadlock recovery, job restart,
hardware integration or arbitrary-model schedulability proofs.

## Interview discussion

Discuss Task/Job separation, policy polymorphism, discrete events vs host threads,
transitive donation, half-open execution intervals, exact-deadline semantics,
RMS sufficient vs necessary conditions, immutable snapshots and why finite trace
success cannot establish indefinite schedulability. Attribute all claimed
results to tests/measurements only after actually running them.
