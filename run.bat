@echo off
setlocal
cd /d "%~dp0"
if not exist ".venv\Scripts\python.exe" (
  echo Run setup.ps1 first. See README.md.
  exit /b 1
)
".venv\Scripts\python.exe" -X utf8 scripts\launch.py
exit /b %errorlevel%
