@echo off
setlocal
call "%~dp0build-MetaHook-x86.bat" Release
exit /b %errorlevel%
