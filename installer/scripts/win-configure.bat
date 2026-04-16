@echo off
REM win-configure.bat  -  bring up MSVC + Qt env, then configure and build installer.
REM Usage: win-configure.bat [configure|build] [preset]
REM Defaults: configure windows-msvc-debug.

setlocal enabledelayedexpansion

set "QTDIR=C:\Qt\6.7.3\msvc2019_64"
set "VCPKG_ROOT=C:\vcpkg"
set "PATH=%QTDIR%\bin;%PATH%"

if not exist "%QTDIR%\bin\qmake.exe" (
    echo [win-configure] Qt not found at %QTDIR% 1>&2
    exit /b 2
)

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%

REM vcvars64 sets VCPKG_ROOT to Visual Studio's bundled vcpkg install. Force
REM back to our standalone C:\vcpkg where libssh2 + openssl live.
set "VCPKG_ROOT=C:\vcpkg"

set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=configure"
set "PRESET=%~2"
if "%PRESET%"=="" set "PRESET=windows-msvc-debug"

pushd "%~dp0\.."

if /i "%ACTION%"=="configure" (
    cmake --preset %PRESET%
) else if /i "%ACTION%"=="build" (
    cmake --build --preset %PRESET%
) else if /i "%ACTION%"=="test" (
    ctest --preset %PRESET%
) else if /i "%ACTION%"=="all" (
    cmake --preset %PRESET% && cmake --build --preset %PRESET%
) else (
    echo [win-configure] unknown action: %ACTION% 1>&2
    popd
    exit /b 2
)

set "RC=%errorlevel%"
popd
exit /b %RC%
