# Metric definitions

All service durations use virtual ticks, not host elapsed milliseconds.

| Export key | Definition / population |
| --- | --- |
| job_time | CPU ticks executing jobs |
| interrupt_time | CPU ticks executing handlers |
| idle_time | CPU ticks with no execution |
| cpu_utilization | (job_time + interrupt_time) / elapsed; 0 at time zero |
| throughput_per_tick | Completed jobs / elapsed |
| average_waiting | Ready-state waiting, completed jobs only |
| average_turnaround | Completion minus release, completed jobs only |
| average_response | First CPU dispatch minus release, dispatched jobs only |
| maximum_response | Maximum first-dispatch delay, dispatched jobs only |
| worst_completed_response | Maximum completion-minus-release, completed jobs only |
| deadline_misses | Unfinished nonterminated jobs when their deadlines are processed |
| evaluated_deadlines | All released job deadlines that have been processed |
| miss_ratio | deadline_misses / evaluated_deadlines; 0 with no eligible deadlines |
| total_blocking | Sum of Blocked-state ticks over all released jobs |
| average_interrupt_latency | First service minus arrival, started handlers only |
| context_switches | Changes between positive-service nonidle job/handler identities, ignoring idle gaps and initial dispatch |
| preemptions | Scheduler/IRQ displacement before completion, plus RR expiration when another task is ready |
| scheduler_decisions | Calls to scheduler select during stable-dispatch resolution |
| watchdog_timeouts | Active-generation expiration events, including alerts at horizon |

An interrupted task is ready while the handler runs. Its delay is counted as
ready waiting, separately from mutex blocking. No task accumulates ready waiting
and blocking simultaneously. A job that misses its deadline can still finish.
Terminated jobs keep their consumed service but are excluded from completed-job
averages. A job terminated before its deadline is not later counted as a miss;
the termination event remains visible for separate interpretation.
