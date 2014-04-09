######################################################################
# Automatically generated by qmake (2.00a) Wed Jun 22 16:57:14 2005
######################################################################

TEMPLATE = lib
CONFIG += qt warn_on
QT -= gui
LIBS += -lz
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += crypt.h \
           ioapi.h \
           quazip.h \
           quazipfile.h \
           quazipfileinfo.h \
           quazipnewinfo.h \
           unzip.h \
           zip.h

SOURCES += ioapi.c \
           quazip.cpp \
           quazipfile.cpp \
           quazipnewinfo.cpp \
           unzip.c \
           zip.c

unix {
  OBJECTS_DIR=.obj
  MOC_DIR=.moc
}

# UNIX installation

isEmpty(PREFIX): PREFIX=/usr/local

unix {
  headers.path=$$PREFIX/include/quazip
  headers.files=$$HEADERS
  target.path=$$PREFIX/lib
  INSTALLS += headers target
}