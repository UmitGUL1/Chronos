# Host threading

SimulationEngine has a single-owner contract: do not call the same instance from
multiple host threads. Simulated tasks are data, not std::threads. Headless runs
execute entirely on the calling thread; independent engines can run concurrently.

The desktop Controller owns a std::jthread. Its worker exclusively owns the engine.
The GUI submits commands through a mutex/condition-variable FIFO bounded to 64
commands. The worker waits without busy loops when paused. Playback chooses the
delay between event batches and cannot change virtual event ordering.

Snapshots are value copies serialized into a private QVariantMap and delivered by
Qt queued invocation. The GUI never receives mutable engine references. At most
one snapshot delivery is outstanding; a refresh flag preserves the newest state
after a coalesced update, even when paused. UI event/interval lists are clipped
to 2,000/5,000 entries; exports retain the complete engine trace.

Destruction requests stop, wakes the worker and joins it before Controller fields
are destroyed. Scheduling comparison passes the stop token into every engine.
Large single-boundary batches and synchronous filesystem export cannot currently
be interrupted in the middle. Stress, ThreadSanitizer and GUI shutdown behavior
still need runtime validation. No claim of race-free certification is made.
