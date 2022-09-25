setlocal

set CURRENT_DIR=%~dp0

cd %CURRENT_DIR%..\
call cmake -S . -B ./build -G "Visual Studio 17 2022"

:: any argument to the script will cause it not to pause
@echo off
IF "%1"=="" (
pause
)
