#!/bin/sh

WZPVERSION=`git describe --abbrev=4 --dirty --always --tags`
NAME=wzplayer-${WZPVERSION}

cd /tmp
git clone 'https://github.com/wilbert2000/wzplayer.git' ${NAME}
tar cvjf ${NAME}.tar.bz2 ${NAME}/
cat ${NAME}/wzplayer.spec | sed -e 's/%define version [a-zA-Z0-9\.]*$/%define version '${WZPVERSION}'/' > /tmp/wzplayer.spec
rm -r /tmp/${NAME}
PCKGDIR=/usr/src/packages/
if [ -e /etc/fedora-release ]; then
    PCKGDIR=/usr/src/redhat/
fi
if [ -e /etc/mandrake-release ]; then
    PCKGDIR=/usr/src/rpm/
fi
cp /tmp/${NAME}.tar.bz2 ${PCKGDIR}SOURCES/
rpmbuild -bb --clean --rmsource wzplayer.spec
