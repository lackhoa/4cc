@echo off
rem Build dist/ and serve it on 5173 (vite preview). Double-click target of the
rem "Autodraw Tablet" desktop shortcut.
rem Replace a running instance: kill the previous run's cmd tree (its PID is
rem in the pid file; window titles aren't matchable under Windows Terminal)
rem and whatever still holds port 5173, then record our own PID.
set PID_FILE=%TEMP%\autodraw-tablet-server.pid
if exist "%PID_FILE%" (
  for /f %%p in ('type "%PID_FILE%"') do taskkill /pid %%p /t /f >nul 2>&1
)
for /f "tokens=5" %%p in ('netstat -ano ^| findstr /r /c:":5173 .*LISTENING"') do taskkill /pid %%p /t /f >nul 2>&1
rem NOTE: for /f runs its command in a throwaway cmd, so our cmd is the grandparent.
for /f %%p in ('powershell -NoProfile -Command "$parent = (Get-CimInstance Win32_Process -Filter ProcessId=$PID).ParentProcessId; (Get-CimInstance Win32_Process -Filter ProcessId=$parent).ParentProcessId"') do echo %%p> "%PID_FILE%"
title autodraw tablet server
cd /d "%~dp0"
echo Building dist/ ...
call npm run build
if errorlevel 1 (
  echo.
  echo BUILD FAILED - not starting the server.
  pause
  exit /b 1
)
echo.
echo Serving dist/ at http://localhost:5173/
call npm run preview
echo.
echo Server exited.
pause
