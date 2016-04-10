#!/bin/sh

NAME=wzplayer

cd /tmp
git clone 'https://github.com/wilbert2000/wzplayer.git' ${NAME}
cd $(NAME)
NAMEVERSION=`git describe --dirty --always --tags`
cd ..
NAMEVERSION=${NAME}-${NAMEVERSION}

mv "$(NAME)" "${NAMEVERSION}"
tar cvjf "${NAMEVERSION}".tar.bz2 "${NAMEVERSION}/"
cat "${NAMEVERSION}/${NAME}.spec" | sed -e 's/%define version [a-zA-Z0-9\.]*$/%define version '${NAMEVERSION}'/' > "/tmp/${NAME}.spec"
rm -r "/tmp/${NAMEVERSION}"
PCKGDIR=/usr/src/packages/
if [ -e /etc/fedora-release ]; then
    PCKGDIR=/usr/src/redhat/
fi
if [ -e /etc/mandrake-release ]; then
    PCKGDIR=/usr/src/rpm/
fi
cp /tmp/${NAME}.tar.bz2 ${PCKGDIR}SOURCES/
# TODO: test quotes
rpmbuild -bb --clean --rmsource ${NAME}.spec
