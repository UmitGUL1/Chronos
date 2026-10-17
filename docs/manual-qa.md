# Desktop acceptance checklist

Run against the extracted Windows ZIP. Record the source_commit from
verification.json, Windows version, display scaling and result for each item.
These checks are pending until a person or supported UI driver performs them.

- Open Start-ChronOS.cmd without a Qt SDK or compiler in PATH.
- Load basic-fcfs, start, pause, step and reset; verify A 0–3 and B 3–5.
- Confirm paused virtual time stays fixed; change playback speed and compare
  completed results to confirm timestamps do not change.
- Visit all nine navigation pages; resize from 1380x900 to 960x650 and maximize.
- Add/update/delete tasks; reject execution 0, deadline 0 and duplicate IDs.
- Load/save JSON in a path containing spaces and non-ASCII characters; reload it.
- Load invalid JSON and invalid resource references; verify a readable error.
- Change algorithm and confirm the run resets; unsupported PI/policy combinations
  must report an error rather than silently alter scenario fields.
- Load priority-inversion and priority-inheritance; compare H completion 12 vs 8.
- Load deadlock; inspect the cycle at time 4. Load nested-interrupts and watchdog.
- Zoom, pan, hover and select timeline jobs/ranges; verify labels match traces.
- Compare policies and inspect side-by-side metrics/timelines.
- Export JSON/CSV/SVG to an empty directory; reject overwriting existing reports.
- Close during playback, pause and comparison; verify no hung background process.
- Automated Linux/Xvfb captures have been visually inspected; two are published.
  Analytics comparison-card controls overlap and need a layout fix. Windows
  interaction, resizing and full-page acceptance remain pending.
