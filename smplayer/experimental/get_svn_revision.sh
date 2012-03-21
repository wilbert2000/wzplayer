#!/bin/sh

# This file is taken from the MPlayer sources, and adapted a little bit.
# It gets the SVN revision number and then saves it in two files:
# src/svn_revision.h and svn_revision

test "$1" && extra="-$1"

svn_revision=`LC_ALL=C svn info 2> /dev/null | grep Revision | cut -d' ' -f2`
test $svn_revision || svn_revision=`cd "$1" && grep revision .svn/entries 2>/dev/null | cut -d '"' -f2`
test $svn_revision || svn_revision=`cd "$1" && sed -n -e '/^dir$/{n;p;q}' .svn/entries 2>/dev/null`
test $svn_revision || svn_revision=0UNKNOWN

NEW_REVISION="${svn_revision}${extra}"
OLD_REVISION=`cat src/svn_revision.h 2> /dev/null | grep '#define SVN_REVISION' | cut -f3 -d' '> /dev/null`

# Update svn_revision.h only on revision changes to avoid spurious rebuilds
# Convert svn_revision.h DOS line endings to UNIX else it appends "\r" when using 'cat'
if test "$NEW_REVISION" != "$OLD_REVISION"; then
    if test "$NEW_REVISION" = "0UNKNOWN" then
        sed -e 's/.$//' -e 's:\$WCREV\$:0:' getrev/svn_revision.h.in > src/svn_revision.h      
        fi
    sed -e 's/.$//' -e 's:\$WCREV\$:'$NEW_REVISION':' getrev/svn_revision.h.in > src/svn_revision.h
    echo "SVN-r${svn_revision}${extra}" > svn_revision
fi
