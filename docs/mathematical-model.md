# Deterministic simulation model

One processor executes at most one job or interrupt. All times are signed 64-bit
integer ticks. A tick is an abstract unit; no physical timer precision is claimed.
Input times are bounded by 1e12; generated deadlines can be at most 2e12.

Jobs have independent IDs, release times, absolute deadlines and remaining
budgets. Task definitions are immutable for one run. Period zero means one
aperiodic release. Execution demand is known exactly; no stochastic demand,
cache, memory bus or I/O model is included.

The event heap is ordered by (time, event kind, insertion sequence). Each step
processes one **timestamp batch**, not one arbitrary member of simultaneous
events. This atomic definition avoids showing impossible intermediate states.
The first step processes time zero without consuming service; tick() processes
initialization and then advances exactly one tick.

At every boundary:

1. Account the half-open interval [old time, new time).
2. Finish active interrupt service; restore the suspended handler if any.
3. Execute due task CPU-offset actions, then complete a job with zero budget.
4. Process queued releases, interrupt arrivals, external heartbeats, mask changes,
   deadline checks and watchdog expirations, in that order.
5. Expire RR quantum and requeue after simultaneous arrivals.
6. Recompute transitive donation, detect deadlocks and choose interrupt/task service.

Completion exactly at a deadline is on time. Heartbeat exactly at watchdog
expiration wins. Mutex actions at the completion offset happen before automatic
completion cleanup. Horizon is a service cutoff: no releases occur at the
horizon; deadline checks and watchdog events at the horizon still run. No job
is forcibly terminated just because the horizon is reached.

Action offsets count **executed job ticks**. For example unlock at offset 6
happens after six CPU ticks, regardless of ready waiting, blocking or interrupts.
Contended lock attempts at offset zero do not themselves consume virtual time.

The invariants include:

- executed + remaining = configured budget for every job, including terminated jobs;
- job service + interrupt service + idle = elapsed simulation time;
- blocked jobs receive no CPU service;
- each mutex has at most one owner and a FIFO waiter list;
- time never decreases and the next positive-service boundary must exceed now.

Input limits: 10,000 task definitions, 100,000 released jobs, 100,000 interrupts,
10,000 resources, 100,000 mask windows, 100,000 external heartbeats, 10,000
actions per task, 8 MiB scenario files. Runtime recordings stop with an explicit
error at two million events or intervals. These bounds do not guarantee that
every permitted worst-case workload is cheap; scheduler scans and donation can
be expensive. See performance.md.
