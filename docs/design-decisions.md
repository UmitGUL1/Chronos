# Design decisions

- **Integer time:** no accumulated floating-point ordering error. Precise input
  bounds allow safe additions without unbounded timestamps.
- **Timestamp batches:** all simultaneous events form one observable transaction.
  Tick stepping uses the same boundary engine; it is not a second simulation.
- **Task/Job separation:** periodic releases may overlap without overwriting state.
- **Strategy interface:** real concrete scheduler objects share a const selection
  contract; RMS reuses fixed-priority dispatch after assigning period ranks.
- **Composition:** engine owns domain state and policy via RAII. Resource and IRQ
  mechanisms are currently cohesive private engine operations rather than separate
  manager classes. Splitting them behind tested interfaces is future refactoring.
- **Scanning policies:** deliberate straightforward implementation, O(J) per
  scheduler selection. Performance claims require measurements before indexing.
- **FIFO mutex wake-up:** independent of priority ranking; PI improves owner
  service and does not secretly change the waiter selection policy.
- **Separate IO target:** JSON and filesystem exports are outside the core.
- **Finite horizon:** no false claim that a short successful run proves indefinite
  schedulability. Pending jobs remain visible and deadlines are explicit.
- **No hidden scenario mutation:** unsupported comparison policies get explanatory
  results instead of silently changing the workload.
- **Bounded traces:** fail explicitly at recording limits rather than dropping
  events and corrupting reported metrics.
- **Dependency approach:** FetchContent uses GoogleTest v1.15.2 and nlohmann/json
  v3.11.3 tags. These are version references, not immutable hash locks. CI actions
  also use release tags. Stronger supply-chain locking is still pending.
