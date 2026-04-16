@echo off
REM run-installer-dev.bat, launch the debug installer with a local tarball
REM override so the GitHub Releases fetch is skipped. Intended for pre-public
REM testing against a real device.
REM
REM Usage:
REM     run-installer-dev.bat [full\path\to\readmarkable-<tag>-chiappa.tar.gz]
REM
REM Defaults to the freshly-staged tarball at the repo root.

setlocal
set "QTDIR=C:\Qt\6.7.3\msvc2019_64"
set "PATH=%QTDIR%\bin;%PATH%"

if "%~1"=="" (
    set "RM_INSTALLER_LOCAL_TARBALL=C:\Projects\ReadMarkable\readmarkable-v1.2.0-dev-chiappa.tar.gz"
) else (
    set "RM_INSTALLER_LOCAL_TARBALL=%~1"
)

if not exist "%RM_INSTALLER_LOCAL_TARBALL%" (
    echo ERROR: tarball not found: %RM_INSTALLER_LOCAL_TARBALL% 1>&2
    exit /b 2
)

echo Using tarball: %RM_INSTALLER_LOCAL_TARBALL%
echo Launching installer...

pushd "%~dp0\..\build\windows-msvc-debug"
start "" readmarkable-installer.exe
popd
endlocal
