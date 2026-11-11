@echo off
title AEGIS Backend Server (C++)
cd /d "%~dp0backend"
echo ===================================================
echo   Starting AEGIS C++ Crow Backend (Port 8080)
echo ===================================================
build\bin\aegis_server.exe ..\database\aegis.db ..\database 8080
pause
