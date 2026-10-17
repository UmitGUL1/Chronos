param(
    [string]$BuildDirectory = 'build/windows-release',
    [string]$OutputDirectory = 'dist/ChronOS'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not $IsWindows) { throw 'This packaging script requires Windows PowerShell 7.' }
$sourceRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = [IO.Path]::GetFullPath((Join-Path $sourceRoot $BuildDirectory))
$packageRoot = [IO.Path]::GetFullPath((Join-Path $sourceRoot $OutputDirectory))
if (-not $packageRoot.StartsWith($sourceRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Package output must be inside the source workspace.'
}
if (Test-Path -LiteralPath $packageRoot) { throw 'Choose a fresh package output directory.' }
if (-not $env:QT_ROOT_DIR) { throw 'QT_ROOT_DIR must identify the Qt kit used to build this application.' }
$qtRoot = $env:QT_ROOT_DIR
$deploy = Join-Path $qtRoot 'bin/windeployqt.exe'
if (-not (Test-Path -LiteralPath $deploy)) { throw 'windeployqt.exe was not found in this Qt kit.' }

& cmake --install $buildRoot --config Release --prefix $packageRoot
if ($LASTEXITCODE -ne 0) { throw 'CMake install failed.' }
$binRoot = Join-Path $packageRoot 'bin'
$desktop = Join-Path $binRoot 'chronos-desktop.exe'
& $deploy --release --qmldir (Join-Path $sourceRoot 'ui/qml') --compiler-runtime --no-translations $desktop
if ($LASTEXITCODE -ne 0) { throw 'Qt runtime deployment failed.' }

# App-local MSVC CRT makes the ZIP independent of a separately installed SDK.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (Test-Path -LiteralPath $vswhere) {
    $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsRoot) {
        $redistRoot = Join-Path $vsRoot 'VC/Redist/MSVC'
        $crtFolders = @(Get-ChildItem -Path (Join-Path $redistRoot '*/x64/Microsoft.VC*.CRT') -Directory -ErrorAction SilentlyContinue | Sort-Object FullName -Descending)
        if ($crtFolders.Count -gt 0) { Copy-Item -Path (Join-Path $crtFolders[0].FullName '*.dll') -Destination $binRoot }
    }
}
foreach ($name in @('Qt6Core.dll','Qt6Gui.dll','Qt6Qml.dll','Qt6Quick.dll','msvcp140.dll','vcruntime140.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $binRoot $name))) { throw "Required runtime missing: $name" }
}
if (-not (Test-Path -LiteralPath (Join-Path $binRoot 'platforms/qwindows.dll'))) { throw 'Windows Qt platform plugin missing.' }

$licenseRoot = Join-Path $packageRoot 'licenses'
New-Item -ItemType Directory -Path $licenseRoot | Out-Null
Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE') -Destination (Join-Path $packageRoot 'LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'licenses/Qt-LGPL-3.0.txt') -Destination $licenseRoot
Copy-Item -LiteralPath (Join-Path $sourceRoot 'licenses/Qt-GPL-3.0.txt') -Destination $licenseRoot
Copy-Item -LiteralPath (Join-Path $buildRoot '_deps/nlohmann_json-src/LICENSE.MIT') -Destination (Join-Path $licenseRoot 'nlohmann-json-MIT.txt')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'docs/windows-package.md') -Destination (Join-Path $packageRoot 'README.txt')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'scripts/Start-ChronOS.cmd') -Destination $packageRoot
Copy-Item -LiteralPath (Join-Path $sourceRoot 'docs/manual-qa.md') -Destination $packageRoot

# Validate only the deployed files, without development Qt or compiler PATHs.
$savedEnvironment = @{}
foreach ($key in @('PATH','QT_PLUGIN_PATH','QML2_IMPORT_PATH','QML_IMPORT_PATH','QT_QPA_PLATFORM','QT_QUICK_BACKEND','QT_FORCE_STDERR_LOGGING')) {
    $savedEnvironment[$key] = [Environment]::GetEnvironmentVariable($key,'Process')
}
$workingFolder = (Get-Location).Path
try {
    $env:PATH = "$binRoot;$env:SystemRoot\System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = ''
    $env:QML2_IMPORT_PATH = ''
    $env:QML_IMPORT_PATH = ''
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QT_QUICK_BACKEND = 'software'
    $env:QT_FORCE_STDERR_LOGGING = '1'
    Set-Location -LiteralPath $packageRoot
    $cliResult = & (Join-Path $binRoot 'chronos-cli.exe') --scenario (Join-Path $packageRoot 'share/chronos/scenarios/priority-inheritance.json')
    if ($LASTEXITCODE -ne 0) { throw 'Packaged CLI failed.' }
    $report = ($cliResult -join "`n") | ConvertFrom-Json
    $highJob = @($report.jobs | Where-Object task -eq 'H')
    if ($highJob.Count -ne 1 -or $highJob[0].completion -ne 8) { throw 'Packaged priority inheritance reference failed.' }
    $process = Start-Process -FilePath $desktop -ArgumentList '--smoke' -WorkingDirectory $packageRoot -WindowStyle Hidden -PassThru -RedirectStandardError (Join-Path $buildRoot 'package-stderr.log') -RedirectStandardOutput (Join-Path $buildRoot 'package-stdout.log')
    if (-not $process.WaitForExit(30000)) {
        $process.Kill()
        throw 'Packaged desktop smoke exceeded 30 seconds.'
    }
    if ($process.ExitCode -ne 0) {
        Get-Content -LiteralPath (Join-Path $buildRoot 'package-stderr.log') -ErrorAction SilentlyContinue | Write-Output
        throw "Packaged desktop smoke failed with exit code $($process.ExitCode)."
    }
} finally {
    Set-Location -LiteralPath $workingFolder
    foreach ($entry in $savedEnvironment.GetEnumerator()) { [Environment]::SetEnvironmentVariable($entry.Key,$entry.Value,'Process') }
}

[ordered]@{
    source_commit = $env:GITHUB_SHA
    configuration = 'Release'
    target = 'Windows x64'
    qt_version = '6.8.3'
    packaged_cli_reference = 'passed: H completes at tick 8'
    packaged_desktop_smoke = 'passed: development Qt/compilers removed from PATH'
    manual_gui_qa = 'pending; see manual-qa.md'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $packageRoot 'verification.json') -Encoding utf8
Write-Output "Verified portable package: $packageRoot"
