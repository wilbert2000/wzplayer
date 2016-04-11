#!/bin/sh
 
ln -s debian-rvm debian
cp debian-rvm/changelog-orig debian-rvm/changelog
dch -v 16.1.0-`git describe --dirty --always --tags` "New version"

#dpkg-buildpackage -rfakeroot

# This should be faster:
rm build-stamp
rm build/wzplayer
fakeroot debian/rules build
fakeroot debian/rules binary

dh_clean
rm debian-rvm/changelog
rm debian
