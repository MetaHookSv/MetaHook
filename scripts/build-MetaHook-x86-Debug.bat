@echo off
setlocal
call "%~dp0build-MetaHook-x86.bat" Debug
exit /b %errorlevel%
