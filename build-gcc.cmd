@ECHO OFF

SET LIB_NAME=sz

SET PRJ_DIR=%~dp0

SET SRC_DIR=src
SET TEST_DIR=test
SET TMP_DIR=.tmp
SET RELEASE_DIR=release


SET CC=g++

::! CL's args are case-sensitive! (LIB's aren't, and I guess LINK is the same...)
SET CPPFLAGS=%CPPFLAGS%
:: -static
SET CPPFLAGS=%CPPFLAGS% -std=c++20 -Wall
::set CPPFLAGS=%CPPFLAGS% -DDEBUG -O2
SET CPPFLAGS=%CPPFLAGS% -DNDEBUG -O2
:: Enable if there are C files, too:
::SET CPPFLAGS=%CPPFLAGS% /Tc src/*.c 

PUSHD %PRJ_DIR%

SET saved_INCLUDE=%INCLUDE%

IF EXIST %TMP_DIR% DEL /S /Q %TMP_DIR%\*
IF NOT EXIST %TMP_DIR% MD %TMP_DIR%

:: Build the lib:
PUSHD %TMP_DIR%
%CC% -DSZ_IMPLEMENTATION -Iinclude -I. -Isrc -c %CPPFLAGS% -x c++ ../%SRC_DIR%/*.c*
FOR %%O IN (*.o) DO (
	ar -rvfuc ../%RELEASE_DIR%/lib%LIB_NAME%.a %%O
)
POPD

:: Build the combined header:
TYPE tooling\header-prefix.h > %RELEASE_DIR%/%LIB_NAME%.h--
FOR /R %SRC_DIR% %%F IN (*.c*); DO (
	TYPE "%%F" >> "%RELEASE_DIR%\%LIB_NAME%.h--"
)
TYPE tooling\header-postfix.h >> %RELEASE_DIR%/%LIB_NAME%.h--

:: Build & run the tests:
::!! `test/run` would be a "fatal" pitfall on Windows: since we have test.cmd
::!! in the current dir, it would mean: `./test.cmd /run`! :-ooo
IF NOT ERRORLEVEL 1 test\run

POPD
