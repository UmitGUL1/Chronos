# Analysis and measured results

The analytical panel supports only independent, fully preemptible periodic
tasks on one processor with D=T, no resource constraints or task actions, no
interrupts and no watchdog recovery. Context-switch cost is modeled as zero.
Scenarios outside these assumptions receive Not applicable rather than a
feasibility claim. Observed finite-horizon success alone never proves feasibility.

Utilization is U = sum(C_i/T_i). For n tasks the RMS sufficient bound is
U <= n(2^(1/n)-1). Passing proves schedulability under the stated model. Exceeding
it is **inconclusive**, not an automatic unschedulability result.

For EDF under the same ideal implicit-deadline single-CPU model, U <= 1 is the
utilization feasibility condition. This implementation refuses to generalize
that check to constrained deadlines, resource blocking or multiprocessors.

Priority inheritance reduces medium-priority interference but does not eliminate
blocking and is not deadlock prevention. The observed blocking measurement is
not an analytical worst-case bound. Fixed-priority response-time recurrence
analysis and demand-bound tests are not implemented.

The field `average_response` is interactive-scheduling first-response delay:
first CPU service minus release. In real-time literature, response time often
means completion minus release. Accordingly `worst_completed_response` reports
the largest observed completion-minus-release value; incomplete jobs are excluded.
