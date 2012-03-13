#!/bin/sh
#svn up
./get_svn_revision.sh

SVN_REVISION=`cat svn_revision | sed -e 's/SVN-/svn_/g'`
VERSION_MAJOR=`LC_ALL=C cat src/svn_revision.h 2> /dev/null | grep '#define VERSION_MAJOR' | cut -f3 -d' '`
VERSION_MINOR=`LC_ALL=C cat src/svn_revision.h 2> /dev/null | grep '#define VERSION_MINOR' | cut -f3 -d' '`
VERSION_BUILD=`LC_ALL=C cat src/svn_revision.h 2> /dev/null | grep '#define VERSION_BUILD' | cut -f3 -d' '`
SMPVERSION=$VERSION_MAJOR.$VERSION_MINOR.$VERSION_BUILD

svn export . /tmp/smplayer-${SMPVERSION}_${SVN_REVISION}
CURDIR=`pwd`
cd /tmp
tar cvjf smplayer-${SMPVERSION}_${SVN_REVISION}.tar.bz2 smplayer-${SMPVERSION}_${SVN_REVISION}/
rm -r /tmp/smplayer-${SMPVERSION}_${SVN_REVISION}
cat ${CURDIR}/smplayer.spec | sed -e 's/%define version [a-zA-Z0-9\.]*$/%define version '${SMPVERSION}'_'${SVN_REVISION}'/' > /tmp/smplayer.spec
PCKGDIR=/usr/src/packages/
if [ -e /etc/fedora-release ]; then
    PCKGDIR=/usr/src/redhat/
fi
if [ -e /etc/mandrake-release ]; then
    PCKGDIR=/usr/src/rpm/
fi
cp /tmp/smplayer-${SMPVERSION}_${SVN_REVISION}.tar.bz2 ${PCKGDIR}SOURCES/
rpmbuild -bb --clean --rmsource smplayer.spec
