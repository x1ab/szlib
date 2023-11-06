@echo off
set TEST_DIR=%~dp0

:: Enable busybox.exe in the prj's tool dir:
set "PATH=%TEST_DIR%..\tooling;%PATH%"

%TEST_DIR%_engine\run %*
