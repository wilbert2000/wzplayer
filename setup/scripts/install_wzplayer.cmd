@echo off
echo This batch file can help you to create a directory with all required files
echo Just change the variables at the beginning
echo.
echo Warning: it will only work with sources from the SVN and the command svn has to be in the path
echo.

set /P QTVER="Qt Version (Default: 4.8.6): "
if "%QTVER%"=="" set QTVER=4.8.6

set OUTPUT_DIR=wzplayer-build

set WZPLAYER_DIR_DIR=wzplayer
set WZPLAYER_DIR_SKINS_DIR=svn\wzplayer-skins
set MPLAYER_DIR=mplayer
set MPV_DIR=mpv
rem set QT_DIR=C:\QtSDK\Desktop\Qt\%QTVER%\mingw
set QT_DIR=C:\Qt\%QTVER%

echo.
echo ######      SMPlayer, QT libs      #######
echo.

mkdir %OUTPUT_DIR%
copy %WZPLAYER_DIR_DIR%\src\release\wzplayer.exe %OUTPUT_DIR%
copy %WZPLAYER_DIR_DIR%\dxlist\release\dxlist.exe %OUTPUT_DIR%
copy %WZPLAYER_DIR_DIR%\zlib\zlib1.dll %OUTPUT_DIR%
copy %WZPLAYER_DIR_DIR%\setup\sample.avi %OUTPUT_DIR%
copy %WZPLAYER_DIR_DIR%\*.txt %OUTPUT_DIR%
copy %QT_DIR%\bin\QtCore4.dll %OUTPUT_DIR%
copy %QT_DIR%\bin\QtGui4.dll %OUTPUT_DIR%
copy %QT_DIR%\bin\QtNetwork4.dll %OUTPUT_DIR%
copy %QT_DIR%\bin\QtXml4.dll %OUTPUT_DIR%
copy %QT_DIR%\bin\QtScript4.dll %OUTPUT_DIR%
copy %QT_DIR%\bin\QtDBus4.dll %OUTPUT_DIR%
copy %QT_DIR%\bin\mingwm10.dll %OUTPUT_DIR%
if %QTVER% geq 4.6.0 (
copy %QT_DIR%\bin\libgcc_s_dw2-1.dll %OUTPUT_DIR%
)
if %QTVER% geq 4.8.0 (
copy %QT_DIR%\bin\libwinpthread-1.dll %OUTPUT_DIR%
copy "%QT_DIR%\bin\libstdc++-6.dll" %OUTPUT_DIR%
)
copy openssl\*.dll %OUTPUT_DIR%

mkdir %OUTPUT_DIR%\imageformats
copy %QT_DIR%\plugins\imageformats\qjpeg4.dll %OUTPUT_DIR%\imageformats\

echo.
echo ######            Fonts            #######
echo.

rem mkdir %OUTPUT_DIR%\open-fonts
rem copy open-fonts\*.* %OUTPUT_DIR%\open-fonts\

echo.
echo ######        Translations         #######
echo.

mkdir %OUTPUT_DIR%\translations
copy %WZPLAYER_DIR_DIR%\src\translations\*.qm %OUTPUT_DIR%\translations
copy %WZPLAYER_DIR_DIR%\qt-translations\*.qm %OUTPUT_DIR%\translations

echo.
echo ######       Qt Translations       #######
echo.
copy %QT_DIR%\translations\qt_*.qm %OUTPUT_DIR%\translations
del %OUTPUT_DIR%\translations\qt_help_*.qm

echo.
echo ######         Shortcuts           #######
echo.
mkdir %OUTPUT_DIR%\shortcuts
copy %WZPLAYER_DIR_DIR%\src\shortcuts\*.keys %OUTPUT_DIR%\shortcuts

echo.
echo ######        Documentation        #######
echo.
svn export --force %WZPLAYER_DIR_DIR%\docs %OUTPUT_DIR%\docs

echo.
echo ######           MPlayer           #######
echo.
xcopy %MPLAYER_DIR% %OUTPUT_DIR%\mplayer\ /E

echo.
echo ######           MPV               #######
echo.
xcopy %MPV_DIR% %OUTPUT_DIR%\mpv\ /E

echo.
