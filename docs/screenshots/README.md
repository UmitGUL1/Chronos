# Actual desktop captures

Captured 17 October 2026 from source `cf540e1d81aee50ecdb3987cce135a1dc51b94c1` in
[GitHub Actions run 39016861075](https://github.com/UmitGUL1/Chronos/actions/runs/39016861075).
These PNGs are the actual 1380x900 Qt 6.8.3 application rendered on Ubuntu/Xvfb
with the Qt Quick software backend. No image generation, mock data, compositing
or visual editing is used.

The default fixed-priority preemption scenario runs to tick 12: A executes
0–2 and 4–7, B executes 2–4, then CPU idle 7–12. Dashboard reports 2 completed
jobs, no deadline misses and 58.33% utilization. Dashboard and Scheduler Lab
are published here. Analytics was also captured, but narrow comparison cards
show overlapping controls; that layout needs a fix before gallery inclusion.
Images were opened for visual inspection before
being added to the repository. This does not replace full manual Windows QA.

Reproduce from the repository root after building the desktop:

```sh
QT_QUICK_BACKEND=software xvfb-run -a -s '-screen 0 1600x1000x24' \
  ./build/screenshots/chronos-desktop --screenshots screenshots
```

The capture command exits nonzero on QML warnings, controller errors, an empty
capture, save failure or a 30-second timeout. Workflow tests run before capture.
