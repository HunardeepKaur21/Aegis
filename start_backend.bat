@echo off
title AEGIS Backend Server
cd /d "%~dp0backend"
echo ===================================================
echo   Starting AEGIS Backend Server (Port 8080)
echo ===================================================

:: If Python is available, run server.py (completely bypasses Device Guard policies)
where python >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    python server.py ..\database\aegis.db ..\database 8080
    goto :end
)

:: Fallback to C++ binary if Python is not found
if exist "build\bin\aegis_server.exe" (
    build\bin\aegis_server.exe ..\database\aegis.db ..\database 8080
)

:end
pause
