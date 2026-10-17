# Performance methodology

Three Windows Release benchmark runs were executed in
[workflow 39016039228](https://github.com/UmitGUL1/Chronos/actions/runs/39016039228)
on source `d5edcb351a40772050b47d44a2a7bf754eaf4e0d`.

| Jobs | Median elapsed ms | Min–max ms (3 samples) |
| --- | --- | --- |
| 10 | 0.0470 | 0.0465–0.0592 |
| 100 | 0.2344 | 0.2270–0.2415 |
| 1,000 | 9.4215 | 9.3630–9.4965 |
| 10,000 | 1073.55 | 1060.33–1083.97 |

Environment: Windows Server 2025 Datacenter, MSVC 19.51.36260.0, Release,
approximately 16 GiB RAM; reported host CPU model AMD EPYC 9V74 80-Core Processor
(the model name does not specify runner-assigned cores). Raw CSV samples and
environment metadata are in `benchmarks/results/windows-release-2026-10-17/`.
These are three samples on a shared runner, not real-time guarantees.
The workload uses FCFS with all one-tick aperiodic jobs released at zero and
simulation horizon equal to job count plus one.

`chronos-benchmark` creates 10, 100, 1,000 and 10,000 one-tick aperiodic jobs. It
measures construction plus simulation with std::chrono::steady_clock, excluding
the final snapshot copy, and prints CSV with elapsed time, recorder events,
events/second, decision count and amortized elapsed microseconds per decision.
That last value is **not isolated scheduler latency**. Dedicated per-decision
latency measurement, peak-memory instrumentation and event-queue profiling remain
unimplemented. Each invocation makes one sample per size; the workflow invokes
it three times, with median/range reported above.

Run from a release build and save output along with CPU model, RAM, OS, compiler
version, dependency versions, build flags and revision. Do not compare Debug
against Release or GUI playback against headless simulation.

The engine scans released jobs for scheduling, wait accounting and donation.
The 10,000-job case can be quadratic. Snapshot serialization copies trace arrays
before UI clipping. These are known optimization targets; correctness reference
tests must be run before and after any change.
