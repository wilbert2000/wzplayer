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
#include <QDebug>
#include "log4qt/logger.h"


namespace Settings {


TRecents::TRecents() : max_items(10) {
}

TRecents::~TRecents() {
}

void TRecents::setMaxItems(int n_items) {

	max_items = n_items;
	while (count() > max_items) {
		removeLast();
	}
}

void TRecents::addItem(QString s, const QString& title) {
    Log4Qt::Logger::logger("Settings::TRecents")->debug(
        "addItem: '" + s + "' '" + title + "'");

	if (!title.isEmpty()) {
		s += "|title]=" + title;
	}

	int pos = indexOf(s);
	if (pos >= 0)
		removeAt(pos);
	prepend(s);

	if (count() > max_items)
		removeLast();
}

QString TRecents::item(int n) {

	QString res;
	QStringList s = (*this)[n].split("|title]=");
	if (s.count() > 0)
		res = s[0];

	return res;
}

QString TRecents::title(int n) {

	QString res;
	QStringList s = (*this)[n].split("|title]=");
	if (s.count() > 1)
		res = s[1];

	return res;
}

void TRecents::fromStringList(const QStringList& list) {

	clear();

	int max = list.count();
	if (max > max_items)
		max = max_items;

	for (int n = 0; n < max; n++) {
		append(list[n]);
	}
}

} // namespace Settings
