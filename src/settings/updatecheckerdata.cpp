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

#include "settings/updatecheckerdata.h"
#include <QSettings>
#include "version.h"


namespace Settings {

TUpdateCheckerData::TUpdateCheckerData() :
	enabled(false),
	days_to_check(7) {
}

TUpdateCheckerData::~TUpdateCheckerData() {
}

void TUpdateCheckerData::save(QSettings* set) {

	set->beginGroup("update_checker");
	set->setValue("checked_date", last_checked);
	set->setValue("enabled", enabled);
	set->setValue("days_to_check", days_to_check);
	set->setValue("last_known_version", last_known_version);
	set->endGroup();
}

void TUpdateCheckerData::load(QSettings* set) {

	set->beginGroup("update_checker");
	last_checked = set->value("checked_date", 0).toDate();
	enabled = set->value("enabled", enabled).toBool();
	days_to_check = set->value("days_to_check", days_to_check).toInt();
	last_known_version = set->value("last_known_version", TVersion::version).toString();
	set->endGroup();
}

} // namespace Settings
