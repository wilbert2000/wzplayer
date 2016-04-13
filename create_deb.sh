#!/bin/sh
 
cp debian/changelog-orig debian/changelog
dch -v `git describe --dirty --always --tags` "New version"

#dpkg-buildpackage -rfakeroot

# This should be faster:
rm build-stamp
make clean
fakeroot debian/rules build
fakeroot debian/rules binary

dh_clean
make clean
rm debian/changelog
