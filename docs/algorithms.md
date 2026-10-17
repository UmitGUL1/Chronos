# Scheduling algorithms

Every policy implements IScheduler::select over read-only job state. Policies
never advance time, own jobs, or access Qt. The engine owns quantum expiration,
releases, blocking and dispatch. The factory selects concrete policy objects.

| Policy | Selection | Preemptive | Information assumption |
| --- | --- | --- | --- |
| FCFS | Oldest ready admission | No task preemption | Arrival known |
| SJF | Smallest original service budget | No task preemption | Entire service demand known |
| SRTF | Smallest remaining service | Yes | Remaining service demand known |
| RR | FIFO, requeue on quantum expiration | Yes | Positive integer quantum |
| Fixed | Largest effective priority | Yes | Integer base priority |
| RMS | Shortest period, using fixed priority strategy | Yes | Every task periodic |
| EDF | Earliest absolute job deadline | Yes | Each job deadline known |

Remaining ties use ready admission sequence and unique job ID. A release at the
same time as RR expiration enters before the expiring job is requeued. If no
other job is eligible, RR immediately resumes the same job without a task
preemption. Interrupt preemption preserves remaining RR quantum. A blocked job
receives a fresh quantum when awakened. Equal fixed priorities retain earlier
ready admission. FCFS/SJF still allow interrupts.

RMS assigns a rank of one plus the count of tasks with longer periods. Equal
periods share a rank. Resource donation uses these ranks. No arbitrary user
priority overrides RMS ordering.

Priority inheritance is supported for fixed/RMS policies. Configurations that
enable it with EDF, RR, SJF, SRTF or FCFS are explicitly rejected, including in
comparison runs. Comparisons never silently change inheritance settings.
