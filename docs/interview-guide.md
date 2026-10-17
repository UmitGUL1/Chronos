# Interview guide

## What is this project?

A deterministic simulator of scheduling/RTOS concepts, not a bootable kernel,
device driver, hardware emulator or timing-certified embedded system. Be candid
about the scope of verification: Windows/Linux builds, core tests, Qt offscreen
startup and ASan/UBSan pass; manual interaction, race testing and profiling remain.

## Why OOP?

IScheduler expresses a small const strategy contract. Concrete strategies can be
replaced without moving the clock or changing resource semantics. Task definitions
and Job runtime instances have distinct responsibilities. Engine ownership is
explicit and held by value/unique_ptr, with RAII cleanup. No manual new/delete
or thread-per-task model is needed.

## Why discrete events?

The next boundary is the minimum queued external event, remaining service, next
CPU-offset action, quantum expiration and horizon. Idle spans jump directly to
the next external event. A one-tick UI operation caps that same boundary function.
Simulation correctness is independent of playback and host OS scheduling.

## How does priority inheritance work?

Start every effective priority at its base value. Repeatedly propagate waiter
priority to resource owners until no value changes. Finite priority values make
this bounded. Recomputing after release restores priorities even with multiple
held locks. Explain the L/M/H trace and how transitive L<-M<-H differs from a
single special-cased inversion demo.

## Does it prevent deadlocks?

No. A blocked job waits on one mutex, so follow wait-for edges until the chain
ends or repeats. Record canonical cycles with their resources/timestamp. Recovery
is not automatic. Watchdog termination is a separate explicitly configured policy.

## How is determinism established?

Integer time, stable IDs, monotonic insertion sequence, documented same-time
ordering, FIFO queues and no dependence on host thread scheduling. Reference
tests compare exact intervals and full reports. They pass on the CI platforms;
passing examples and bounded invariant checks are evidence, not a universal proof.

## What would you improve?

Keep the full CI matrix passing. Then profile ready scans and snapshot copies, extract resource/interrupt
managers if growth warrants it, add isolated scheduler latency/peak memory
measurements, more GUI tests, dependency hashes and verified release packaging.
Do not invent benchmark or coverage figures in an interview.
