# Resources, deadlocks, interrupts and watchdog

Mutexes are exclusive and nonrecursive. Lock/unlock actions are validated against
a per-task structural acquisition sequence. Holding a lock through completion is
allowed: completion releases owned resources in deterministic resource-name order.
Ownership transfers directly to the oldest waiter. The blocked lock action is
consumed before blocking and is not retried on wake-up. Foreign unlock is an error.

Effective priorities start at base priority and reach a fixed point by donating
each waiter's effective priority to the owner of the awaited resource. This
propagates across nested dependencies and handles multiple waiters/owned locks.
Recomputation from base restores priorities after release. The same general
mechanism handles the demonstration; no scenario names are special-cased.

A wait-for edge is waiting job -> owning job. Outdegree is at most one in this
exclusive-mutex model. Cycle detection follows chains, canonicalizes cycles by
smallest job ID and records each cycle once with time and resource names. This
is historical detection, not recovery; there is no automatic deadlock resolution.

Interrupts have pending, active, suspended and completed states. All interrupts
outrank tasks. Larger interrupt priorities outrank smaller ones. A strictly
higher-priority handler can nest if enabled; equal priorities do not nest. Pending
ties use arrival then configuration order. A LIFO stack resumes suspended handlers.
Mask windows [start,end) defer new handlers; active handlers continue. Interrupts
have no mutex actions and cannot block. Their service contributes to utilization
and delays job completion, potentially causing deadline misses.

The watchdog is a **subsystem** watchdog, using virtual time. External heartbeat
events or task heartbeat actions invalidate older expiration generations. An
active timeout emits an alert and rearms. The optional terminate_jobs policy
terminates all currently released unfinished jobs and cleans up locks/wait lists.
It does not restart terminated jobs or cancel future periodic releases. Simulated
restart/recovery beyond this explicit termination policy is not implemented.
