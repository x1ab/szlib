@echo off

set TEST_DIR=.\test
:: Just run the latest test/*.exe, whatever flavor it is...
for /f %%f in ('dir /b /o-d /t:w "%TEST_DIR%\*.exe"') do (
	set "latest_exe=%%f"
	goto :found
)
echo - No suitable test executable found.
goto :eof

:found
echo Launching: %TEST_DIR%\%latest_exe% %*...
"%TEST_DIR%\%latest_exe%" %*
