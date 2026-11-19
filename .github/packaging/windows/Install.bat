@echo off
setlocal
title Clean Glides - VST3 installer

set "NAME=Clean Glides.vst3"
set "SRC=%~dp0%NAME%"
set "DEST=%CommonProgramFiles%\VST3"

net session >nul 2>&1
if not "%errorlevel%"=="0" (
    echo Requesting administrator rights...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

if not exist "%SRC%" (
    echo ERROR: "%NAME%" was not found next to this script.
    echo Extract the whole ZIP archive first, then run Install.bat again.
    pause
    exit /b 1
)

set "MACHINE="
for /f "tokens=3" %%A in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PROCESSOR_ARCHITECTURE 2^>nul ^| find "REG_SZ"') do set "MACHINE=%%A"

echo Installing to: %DEST%\%NAME%
if not exist "%DEST%" mkdir "%DEST%"
if exist "%DEST%\%NAME%" rmdir /s /q "%DEST%\%NAME%"
xcopy "%SRC%" "%DEST%\%NAME%\" /E /I /Y /Q >nul
if errorlevel 1 (
    echo ERROR: the installation failed.
    pause
    exit /b 1
)

rem On an x64 PC the ARM64 build can never be used, so it is removed.
rem On an ARM PC both builds are kept: most DAWs there are x64 programs
rem that need the x64 build, and only native ARM64 DAWs need the other one.
set "DROPPED="
if /i "%MACHINE%"=="AMD64" if exist "%SRC%\Contents\x86_64-win" if exist "%DEST%\%NAME%\Contents\arm64-win" (
    rmdir /s /q "%DEST%\%NAME%\Contents\arm64-win"
    set "DROPPED=1"
)

if defined DROPPED (
    echo This PC is x64: installed the x64 build only.
) else (
    echo Installed every build included in the archive.
)

echo.
echo Clean Glides was installed successfully.
echo Restart your DAW and rescan your plugins if needed.
echo.
pause