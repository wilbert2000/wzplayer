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


#ifndef SETTINGS_UPDATE_CHECKER_DATA_H
#define SETTINGS_UPDATE_CHECKER_DATA_H

#include <QString>
#include <QDate>

class QSettings;

namespace Settings {

class TUpdateCheckerData {
public:
    TUpdateCheckerData();
    virtual ~TUpdateCheckerData();

    void save(QSettings* set);
    void load(QSettings* set);

    QDate last_checked;
    bool enabled;
    int days_to_check;
    QString last_known_version;
};

} // namespace Settings

#endif // SETTINGS_UPDATE_CHECKER_DATA_H
