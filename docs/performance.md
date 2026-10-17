# Performance methodology

No C++ benchmark was executed in the authoring environment. There are no measured
runtime, memory, throughput, or scheduling-latency claims in this release.

`chronos-benchmark` creates 10, 100, 1,000 and 10,000 one-tick aperiodic jobs. It
measures construction plus simulation with std::chrono::steady_clock, excluding
the final snapshot copy, and prints CSV with elapsed time, recorder events,
events/second, decision count and amortized elapsed microseconds per decision.
That last value is **not isolated scheduler latency**. Dedicated per-decision
latency measurement, peak-memory instrumentation and event-queue profiling remain
unimplemented. The benchmark currently makes one sample per size; use repeated
release-build runs and report median/range for an actual study.

Run from a release build and save output along with CPU model, RAM, OS, compiler
version, dependency versions, build flags and revision. Do not compare Debug
against Release or GUI playback against headless simulation.

The engine scans released jobs for scheduling, wait accounting and donation.
The 10,000-job case can be quadratic. Snapshot serialization copies trace arrays
before UI clipping. These are known optimization targets; correctness reference
tests must be run before and after any change.
