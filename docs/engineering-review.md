# Engineering review — CI-verified development preview

## Outcome and evidence

The C++20 core, CLI and Qt/QML desktop build on Windows/MSVC and Ubuntu/GCC.
Successful run [39014818010](https://github.com/UmitGUL1/Chronos/actions/runs/39014818010)
verifies revision b43a10f30f590b13d58a9857254ddcd4857b50eb.
Core CTest suites pass 44 entries each; Qt suites pass 45 including offscreen
startup; Linux ASan/UBSan passes 44. See ci-verification.md for compiler versions,
test timings and the precise limits of this evidence.

The sources remain a development preview, with full interactive/visual testing,
benchmark measurements and distributable runtime packaging still pending.

## Engineering assessment

- Engine values and unique_ptr policy provide explicit ownership; jthread is
  joined before Controller fields are destroyed.
- Integer ticks, stable event order and CPU-offset actions separate virtual time
  from host timing. Exact-deadline and heartbeat tie semantics are documented.
- Concrete scheduler strategies share a const interface; RMS assigns period
  ranks before reusing fixed-priority selection.
- Exclusive/FIFO mutex ownership, fixed-point donation and cycle detection are
  general mechanisms without scenario-specific scheduling exceptions.
- Nested/pending interrupts and subsystem watchdog use the same virtual clock.
- Population-specific metrics and guarded RMS/EDF checks avoid claiming a proof
  from finite-horizon observations or applying utilization tests outside scope.
- GUI consumes immutable queued snapshots; every metric and chart uses engine
  reports. Resource/interrupt details currently use inspection panels.
- Inputs are bounded; exports refuse existing report names and escape CSV/SVG.

## Defects addressed

Source review fixed RR preemption accounting and coalesced snapshot refresh.
Actual CI then found a missing namespace closure, incorrect explicit Qt package
architecture and an assignment to Flow's read-only implicitHeight property.
Those issues were fixed and all five matrix jobs passed. Windows console
diagnostics were retained so subsequent desktop failures appear in CTest logs.

## Remaining material limitations

Current tests are meaningful references and invariants, not an exhaustive proof.
ASan/UBSan does not establish absence of data races. Manual controls, dialogs,
resizing, visual hierarchy, exports through the UI and shutdown under load still
need interaction testing. No actual screenshots have been captured.

Ready/wait/donation scans can be quadratic. Snapshot serialization copies full
history before clipping for display. Export is synchronous in the worker and
cannot be interrupted within one operation. There are no benchmark measurements,
isolated scheduling latency or peak-memory results. Dependency/action versions
use release tags rather than immutable hashes.

One ideal CPU and zero switching overhead are modeled. There is no production
RTOS kernel, multicore scheduling, general deadlock recovery, job restart,
hardware integration or arbitrary-model schedulability proof. Qt runtime release
bundles and full GUI test automation remain pending.

## Interview discussion

Explain Task/Job separation, strategy polymorphism, discrete events vs host
threads, transitive donation, interval accounting, RMS sufficient conditions,
immutable snapshots and the distinction between tested references and proof.
Use the linked actual CI results, and do not invent performance/coverage claims.
