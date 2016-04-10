
cd zlib
mingw32-make -fwin32\makefile.gcc
cd ..

cd src
lrelease wzplayer.pro
qmake DEFINES+=PORTABLE_APP
mingw32-make
