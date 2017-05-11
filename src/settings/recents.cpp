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

#include "settings/recents.h"


namespace Settings {

const QString TITLE_SEP = "|title]=";

TRecents::TRecents() {
}

TRecents::~TRecents() {
}

void TRecents::addRecent(QString s, const QString& title) {

    s += TITLE_SEP;

    // Remove existing item
    {
        QStringList::iterator iterator = begin();
        while (iterator != end()) {
            if ((*iterator).startsWith(s)) {
                erase(iterator);
                break;
            }
            iterator++;
        }
    }

    prepend(s + title);

    if (count() > getMaxItems()) {
        removeLast();
    }
}

QString TRecents::getURL(int n) {

    QString res;
    QStringList s = (*this)[n].split(TITLE_SEP);
    if (s.count() > 0) {
        res = s[0];
    }

    return res;
}

QString TRecents::getTitle(int n) {

    QString res;
    QStringList s = (*this)[n].split(TITLE_SEP);
    if (s.count() > 1) {
        res = s[1];
    }

    return res;
}

} // namespace Settings
