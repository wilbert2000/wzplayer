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

#ifndef SETTINGS_RECENTS_H
#define SETTINGS_RECENTS_H

#include "settings/lrulist.h"


namespace Settings {

class TRecents : public TLRUList {
public:
    TRecents();

    void addRecent(QString url, const QString& title);

    QString getURL(int n);
    QString getTitle(int n);
};

} // namespace Settings

#endif // SETTINGS_RECENTS_H
