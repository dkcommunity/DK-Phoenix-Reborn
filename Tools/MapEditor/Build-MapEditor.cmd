@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-MapEditor.ps1" %*
exit /b %errorlevel%
