@echo off
if exist "turnon_echo" (
  @echo on
)

:: Reset working dir especially when using 'Run as administrator'
@cd /d "%~dp0"

echo This batch file can help you to create a packages for WZPlayer and MPlayer.
echo.
echo This script will temporarily rename the wzplayer-build and mplayer directories.
echo Make sure these files ^& directories are not opened when running the script.
echo.
echo 1 - NSIS [64-bit]
echo 2 - Without MPlayer [64-bit]
echo.

:: Root dir op wzplayer project
set PROJECT_DIR=..

:: Reset in case ran again in same command prompt instance
set ALL_PKG_VER=
set VER_MAJOR=
set VER_MINOR=
set VER_BUILD=
set VER_REVISION=
set VER_REV_CMD=
set MAKENSIS_EXE_PATH=

:: NSIS path
if exist "nsis_path" (
  for /f "tokens=*" %%y in ('type nsis_path') do if exist %%y set MAKENSIS_EXE_PATH="%%y"
)

if not defined MAKENSIS_EXE_PATH (
  for %%x in ("%PROGRAMFILES(X86)%\NSIS\Unicode\makensis.exe" "%PROGRAMFILES%\NSIS\Unicode\makensis.exe") do if exist %%x set MAKENSIS_EXE_PATH=%%x
)

if not defined MAKENSIS_EXE_PATH (
  echo Warning: Unable to locate NSIS in the default path, create the file ^'nsis_path^' with the full correct path
  echo to makensis.exe or the existing ^'nsis_path^' may be incorrect.
  echo.
)

:: Works only in Vista+
where /q where.exe 2>NUL && (
  where /q 7za.exe 2>NUL || (
  echo Warning: 7za.exe not found in path or current directory!
  echo.
  )
)

set BUILD_DIR64=%PROJECT_DIR%\wzplayer-build64
set MPLAYER_DIR=%PROJECT_DIR%\mplayer
set OUTPUT_DIR=%PROJECT_DIR%\output

:cmdline_parsing
if "%1" == ""               goto reask
for %%w in (1 2) do (
  if "%1" == "%%w" (
    set USER_CHOICE=%%w
    goto pkgver
  )
)

echo Unknown option: "%1"
echo.
goto superend

:reask
set USER_CHOICE=
set /P USER_CHOICE="Choose an action: "
echo.

for %%z in (1 2) do if "%USER_CHOICE%" == "%%z" goto pkgver
if "%USER_CHOICE%" == ""  goto superend
goto reask

:pkgver
if exist "pkg_version" (
  for /f "tokens=*" %%i in ('type pkg_version') do set ALL_PKG_VER=%%i
  goto parse_version
)

echo Format: VER_MAJOR.VER_MINOR.VER_BUILD[.VER_REVISION]
echo VER_REVISION is optional (set to 0 if blank)
:pkgver_manual
echo.
set ALL_PKG_VER=
set VER_MAJOR=
set VER_MINOR=
set VER_BUILD=
set VER_REVISION=
set VER_REV_CMD=
set DEF_QT5=
set /p ALL_PKG_VER="Version: "
echo.

:parse_version
for /f "tokens=1 delims=." %%j in ("%ALL_PKG_VER%")  do set VER_MAJOR=%%j
for /f "tokens=2 delims=." %%k in ("%ALL_PKG_VER%")  do set VER_MINOR=%%k
for /f "tokens=3 delims=." %%l in ("%ALL_PKG_VER%")  do set VER_BUILD=%%l
for /f "tokens=4 delims=." %%m in ("%ALL_PKG_VER%")  do set VER_REVISION=%%m

echo %VER_MAJOR%|findstr /r /c:"^[0-9][0-9]*$" >nul
if errorlevel 1 (
  echo Invalid version string. VER_MAJOR is not defined or is not a number [#.x.x]
  goto pkgver_manual & ver>nul
)

echo %VER_MINOR%|findstr /r /c:"^[0-9][0-9]*$" >nul
if errorlevel 1 (
  echo Invalid version string. VER_MINOR is not defined or is not a number [x.#.x]
  goto pkgver_manual & ver>nul
)
echo %VER_BUILD%|findstr /r /c:"^[0-9][0-9]*$" >nul
if errorlevel 1 (
  echo Invalid version string. VER_BUILD is not defined or is not a number [x.x.#]
  goto pkgver_manual & ver>nul
)

if defined VER_REVISION (
  echo %VER_REVISION%|findstr /r /c:"^[0-9][0-9]*$" >nul
  if errorlevel 1 (
    echo Invalid version string. VER_REVISION is not a number [x.x.x.#]
    goto pkgver_manual & ver>nul
  ) else (
    set VER_REV_CMD=/DVER_REVISION=%VER_REVISION% & ver>nul
  )
) else (
  set VER_REV_CMD=
)

if not defined VER_REVISION (
  set PORTABLE_PKG_VER=%ALL_PKG_VER%.0
) else (
  set PORTABLE_PKG_VER=%ALL_PKG_VER%
)

if "%USER_CHOICE%" == "1"  goto nsispkg64
if "%USER_CHOICE%" == "2"  goto nomplayer64
:: Should not happen
goto end

:nsispkg64
echo --- WZPlayer NSIS Package [64-bit] ---
echo.

set DEF_QT5="/DQT5"

if exist %PROJECT_DIR%\wzplayer-build64 (
  %MAKENSIS_EXE_PATH% /V3 /DVER_MAJOR=%VER_MAJOR% /DVER_MINOR=%VER_MINOR% /DVER_BUILD=%VER_BUILD% %VER_REV_CMD% /DWIN64 %DEF_QT5% %PROJECT_DIR%\wzplayer.nsi
)

goto end


:nomplayer64
echo --- Creating WZPlayer without MPlayer Package [64-bit] ---
echo.

ren %BUILD_DIR64% wzplayer-%ALL_PKG_VER%-x64
set WZPLAYER_NOMP_DIR=%PROJECT_DIR%\wzplayer-%ALL_PKG_VER%-x64

::
echo Finalizing package...
7za a -t7z %OUTPUT_DIR%\wzplayer-%ALL_PKG_VER%-x64_without_mplayer.7z %WZPLAYER_NOMP_DIR% -xr!mplayer -mx9 >nul

ren %WZPLAYER_NOMP_DIR% wzplayer-build64

goto end

:end

pause

:superend
set ALL_PKG_VER=
set VER_MAJOR=
set VER_MINOR=
set VER_BUILD=
set VER_REVISION=
