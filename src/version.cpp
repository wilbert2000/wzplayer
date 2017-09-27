/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "version.h"
#include <QRegExp>


const QString TVersion::version(WZPLAYER_VERSION_STR);

int TVersion::qtVersion() {

    QRegExp rx("(\\d+)\\.(\\d+)\\.(\\d+)");
    QString v(qVersion());

    int r = 0;
    if (rx.indexIn(v) >= 0) {
        int n1 = rx.cap(1).toInt();
        int n2 = rx.cap(2).toInt();
        int n3 = rx.cap(3).toInt();
        r = n1 * 1000 + n2 * 100 + n3;
    }

    return r;
}
