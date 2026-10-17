# Architecture

The Qt-free `chronos-core` target contains domain types, concrete policy objects
behind IScheduler and the SimulationEngine. `chronos-io` adds nlohmann/json,
validation, filesystem configuration, comparison and report exports. CLI and Qt
desktop link to IO. The core has no Qt or JSON headers in its public engine model.

Task is an immutable definition for a run. Job is a released runtime instance with
its own execution/state/deadline. ResourceView and InterruptView are value records.
The engine owns a timestamp-ordered priority queue, simulated processor state,
mutex owners/waiters, interrupt stack, trace, watchdog generation and counters.
Resource/interrupt mechanisms currently live as cohesive private operations of
the engine; dedicated manager classes are not claimed as implemented.

Scheduling policy objects only select a runnable job. They do not mutate the
clock. RMS assigns period-derived static ranks and uses the fixed-priority
dispatcher. Round Robin quantum accounting belongs to the processor/engine
boundary model. The same engine supports headless run, event-batch stepping and
one-tick stepping.

The desktop Controller has a GUI-owned presentation state and a worker-owned
engine. Commands cross a bounded mutex/CV queue; immutable value snapshots return
through queued Qt invocations. No engine pointer is exposed to QML. Comparison
runs use independent engines with identical configurations, changing only policy.

Files:

- include/chronos/model.hpp: domain records and validation/analysis contracts.
- include/chronos/scheduler.hpp and src/scheduling/: policy interface/strategies.
- include/chronos/engine.hpp and src/simulation/: deterministic coordinator.
- include/chronos/io.hpp and src/configuration/: persistence/report boundary.
- apps/desktop/: Qt worker and presentation adapter.
- ui/qml/: actual engine-backed pages, chart and wait graph.
- tests/: reference, persistence, replay and host-concurrency tests.

Detailed event order, invariants and limits are in mathematical-model.md;
tradeoffs in design-decisions.md; architecture/UML/event/state diagrams in
diagrams.md. No class diagram depicts unimplemented manager abstractions.
