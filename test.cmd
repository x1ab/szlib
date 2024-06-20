@echo off

set RUN_DIR=.
:: Just run the latest test/oon*.exe, whatever flavor it is...
for /f %%f in ('dir /b /o-d /t:w "%RUN_DIR%\*test*.exe"') do (
	set "latest_exe=%%f"
	goto :found
)
goto :eof

:found
echo Launching: %RUN_DIR%\%latest_exe% %*...
"%RUN_DIR%\%latest_exe%" %*
