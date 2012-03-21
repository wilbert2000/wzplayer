@echo off

:: Undefine for mulitple runs
set Revision=
for /f "tokens=2" %%G IN ('svn info ^| find "Revision: "') do set /a Revision=%%G

:: Set to 0 if unknown (no svn or working copy)
if "%REVISION%"=="" (
  set REVISION=0
)

if "%Revision%"=="0" (
echo Unknown SVN revision. SVN missing in PATH or not a working copy.
) else (
echo SVN Revision: %Revision%
)
echo.

:: Use "Fake SED"
call getrev\fsed.cmd $WCREV$ %Revision% getrev\svn_revision.h.in>src\svn_revision.h

:: Use GNU SED Win32
:: sed s:\$WCREV\$:%Revision%: getrev\svn_revision.h.in>src\svn_revision.h