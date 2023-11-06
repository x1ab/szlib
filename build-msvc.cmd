@ECHO OFF

SET LIB_NAME=sz

SET PRJ_DIR=%~dp0

SET SRC_DIR=src
SET TEST_DIR=test
SET TMP_DIR=.tmp
SET RELEASE_DIR=release


::! CL's args are case-sensitive! (LIB's aren't, and I guess LINK is the same...)
SET CPPFLAGS=%CPPFLAGS% /MT
SET CPPFLAGS=%CPPFLAGS% /std:c++latest /W4
::set CPPFLAGS=%CPPFLAGS% -DDEBUG -O2
SET CPPFLAGS=%CPPFLAGS% /DNDEBUG -O2
SET CPPFLAGS=%CPPFLAGS% /EHsc
:: Enable if there are C files, too:
::SET CPPFLAGS=%CPPFLAGS% /Tc src/*.c 

PUSHD %PRJ_DIR%

SET saved_INCLUDE=%INCLUDE%
SET INCLUDE=include;.;src;%INCLUDE%

IF EXIST %TMP_DIR% DEL /S /Q %TMP_DIR%\*
IF NOT EXIST %TMP_DIR% MD %TMP_DIR%

:: Build the lib:
CL /nologo /DSZ_IMPLEMENTATION /c %CPPFLAGS% /Tp %SRC_DIR%/*.c* /Fo%TMP_DIR%/ /Fd%TMP_DIR%/
LIB /NOLOGO %TMP_DIR%/*.obj /OUT:%RELEASE_DIR%/%LIB_NAME%.lib
::	/REMOVE:main

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
