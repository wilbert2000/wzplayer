@echo off

set startdir=%CD%

set script_name=%0
set qmake_defs=

:arg_loop
if [%1]==[] (

  goto compile

) else if [%1]==[pe] (

  set qmake_defs=PORTABLE_APP

) else if [%1]==[-h] (

  echo How to use:
  echo.
  echo Add ^`pe^' to compile the portable version
  echo.
  echo To compile the normal WZPlayer, enter no arguments.
  goto end

) else (

  echo configure: error: unrecognized option: `%1'
  echo Try `%script_name% -h' for more information
  goto end

) 

shift
goto arg_loop

:compile

cd zlib
mingw32-make -fwin32\makefile.gcc

cd ..\src
lrelease wzplayer.pro
qmake "DEFINES += %qmake_defs%"
mingw32-make

if [%errorlevel%]==[0] (
  echo done
) else (
echo Compilation error, script aborted
:: Resets errorlevel to 0
ver >nul
)

:end
