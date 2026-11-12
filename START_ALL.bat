@echo off
title AEGIS System Launcher
color 0A
echo ========================================================
echo         AEGIS: Advanced Emergency Intelligence
echo ========================================================
echo.
echo [1/3] Starting Backend Server (Port 8080)...
start "AEGIS Backend Server" cmd /k "cd /d "%~dp0" && start_backend.bat"

echo [2/3] Waiting for Backend initialization...
timeout /t 2 /nobreak >nul

echo [3/3] Starting React Frontend (Port 5173)...
start "AEGIS Frontend (React Vite)" cmd /k "cd /d "%~dp0frontend" && npm run dev"

timeout /t 3 /nobreak >nul
echo.
echo Opening AEGIS Dashboard in browser: http://localhost:5173 ...
start http://localhost:5173

echo.
echo ========================================================
echo   AEGIS is now running! Keep both command windows open.
echo   - Backend:  http://localhost:8080
echo   - Frontend: http://localhost:5173
echo   - Database: aegis\database\aegis.db
echo ========================================================
echo.
pause
