ChronOS — Windows x64 development preview

1. Extract this entire ZIP to a folder. Do not run the EXE inside the ZIP viewer.
2. Double-click Start-ChronOS.cmd, or open bin/chronos-desktop.exe.
3. Open Scenario Library and load a bundled scenario. Start / Resume begins
   playback; Pause / Stop pauses; Reset clears the runtime state.

Requirements: Windows 10/11 x64. Qt libraries, QML plugins and MSVC runtime DLLs
are included. No Qt SDK, CMake or compiler installation should be needed.

The bundled CLI is bin/chronos-cli.exe. Example from the extracted folder:

bin\chronos-cli.exe --scenario share\chronos\scenarios\priority-inheritance.json

Save/export dialogs need a writable location. Export requires a fresh directory.
The package is unsigned; its SHA-256 manifest and source commit identify this
build. This simulator is a development preview, not a production RTOS kernel.

The measurements/ directory contains three actual Release benchmark CSV runs
for 10, 100, 1,000 and 10,000 jobs plus environment.json with runner hardware,
compiler and source revision. These measurements are workload-specific, not
real-time guarantees or isolated scheduler latency measurements.

Automated validation includes Release CTest and running the deployed CLI and
desktop smoke with development Qt/compiler paths removed. This does not replace
manual mouse/keyboard, file-dialog, visual or resizing checks. See manual-qa.md.

Project source: https://github.com/UmitGUL1/Chronos
Qt version: 6.8.3, dynamically linked. Qt licensing and source:
https://www.qt.io/licensing/open-source-lgpl-obligations
https://code.qt.io/cgit/qt/qtbase.git/tag/?h=v6.8.3
https://code.qt.io/cgit/qt/qtdeclarative.git/tag/?h=v6.8.3
The library DLLs may be replaced by compatible builds. Qt license text and the
nlohmann/json MIT notice are provided in licenses/. ChronOS itself is MIT.
