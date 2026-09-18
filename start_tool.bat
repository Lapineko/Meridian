@echo off
setlocal
cd /d "%~dp0"
if exist "%~dp0release\Meridian\Meridian.exe" (
    cd /d "%~dp0release\Meridian"
    start "" "Meridian.exe"
    exit /b
)
if exist "%~dp0Meridian.exe" (
    cd /d "%~dp0"
    start "" "Meridian.exe"
    exit /b
)
echo Please extract the portable release, then run Meridian.exe.
echo To build from source, see README.md.
pause
