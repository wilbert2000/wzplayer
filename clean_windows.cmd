cd zlib
mingw32-make -fwin32\makefile.gcc clean

cd ..\src
mingw32-make distclean

cd ..
del src\translations\*.qm
del src\object_script.wzplayer.Release
del src\object_script.wzplayer.Debug
rd src\release
rd src\debug
