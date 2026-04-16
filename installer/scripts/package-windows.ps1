# package-windows.ps1  -  produce ReadMarkableInstaller-setup.exe from a
# windows-msvc-release build.
#
# Prereqs:
#   - QTDIR set to C:\Qt\6.7.3\msvc2019_64
#   - NSIS installed (nsis on PATH, or C:\Program Files (x86)\NSIS\makensis.exe)
#   - The release build already compiled:
#       installer\scripts\win-configure.bat all windows-msvc-release
#
# Output:
#   - installer\ReadMarkableInstaller-setup.exe
#   - installer\ReadMarkableInstaller-setup.exe.sha256

param(
    [string]$BuildDir = "$PSScriptRoot\..\build\windows-msvc-release",
    [string]$QtDir    = $env:QTDIR
)

$ErrorActionPreference = "Stop"

if (-not $QtDir) { $QtDir = "C:\Qt\6.7.3\msvc2019_64" }
if (-not (Test-Path "$QtDir\bin\windeployqt.exe")) {
    throw "windeployqt.exe not found at $QtDir\bin. Set QTDIR."
}

$exe = Join-Path $BuildDir "readmarkable-installer.exe"
if (-not (Test-Path $exe)) {
    throw "Executable not found: $exe. Did you run cmake --build for windows-msvc-release?"
}

# Stage Qt dependencies into installer\build\deploy-win\
$deployDir = Join-Path $PSScriptRoot "..\build\deploy-win"
if (Test-Path $deployDir) { Remove-Item -Recurse -Force $deployDir }
New-Item -ItemType Directory -Path $deployDir | Out-Null
Copy-Item $exe $deployDir

Write-Host "[package] Running windeployqt..."
& "$QtDir\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw (Join-Path $deployDir "readmarkable-installer.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

# Locate NSIS.
$makensis = (Get-Command makensis.exe -ErrorAction SilentlyContinue).Source
if (-not $makensis) {
    $candidate = "C:\Program Files (x86)\NSIS\makensis.exe"
    if (Test-Path $candidate) { $makensis = $candidate }
}
if (-not $makensis) { throw "makensis.exe not found on PATH nor C:\Program Files (x86)\NSIS\. Install NSIS." }

$nsi = Join-Path $PSScriptRoot "installer.nsi"
Write-Host "[package] Running NSIS: $makensis $nsi"
& $makensis $nsi
if ($LASTEXITCODE -ne 0) { throw "NSIS compile failed" }

$setup = Join-Path $PSScriptRoot "..\ReadMarkableInstaller-setup.exe"
if (-not (Test-Path $setup)) { throw "NSIS did not produce expected output: $setup" }

# Compute sha256 for publishing next to the GH Release asset.
$hash = (Get-FileHash -Algorithm SHA256 $setup).Hash.ToLower()
Set-Content -NoNewline -Path "$setup.sha256" -Value "$hash  ReadMarkableInstaller-setup.exe"

Write-Host "[package] OK"
Write-Host "  Artifact: $setup"
Write-Host "  SHA-256:  $hash"
