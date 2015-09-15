/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "titletracks.h"

#include "helper.h"

namespace Maps {

TTitleData::TTitleData() :
	TData(),
	chapters(),
	name(),
	duration(-1),
	angles(-1),
	isTrack(false) {
}

QString TTitleData::getDisplayName(bool add_duration) const {

	QString dname = QString::number(ID);

	if (name.isEmpty()) {
		// TODO: translate
		if (isTrack) {
			dname = "Track " + dname;
		} else {
			dname = "Title " + dname;
		}
	} else {
		dname += " " + name;
	}

	if (add_duration && duration >= 0) {
		dname += " (" + Helper::formatTime(qRound(duration)) + ")";
	}

	return dname;
}

void TTitleTracks::addName(int ID, const QString &name) {
	TTitleData& title = (*this)[ID];
	title.setID(ID);
	title.setName(name);
}

void TTitleTracks::addDuration(int ID, double duration, bool is_track) {
	TTitleData& title = (*this)[ID];
	title.setID(ID);
	title.setDuration(duration);
	title.setType(is_track);
}

void TTitleTracks::addTrack(int ID, const QString &name, double duration) {
	(*this)[ID].setTrack(ID, name, duration);
}

void TTitleTracks::addAngles(int ID, int n) {
	TTitleData& title = (*this)[ID];
	title.setID(ID);
	title.setAngles(n);
}

void TTitleTracks::list() const {

	qDebug("TitleTracks::list: selected ID: %d", selectedID);
	TTitleTrackIterator i(*this);
	while (i.hasNext()) {
		i.next();
		TTitleData d = i.value();
		qDebug("TitleTracks::list: ID: %d name: '%s' duration %f chapters: %d angles: %d",
			   d.getID(), d.getName().toUtf8().constData(), d.getDuration(),
			   d.chapters.count(), d.getAngles());
	}
}

} // namespace Maps
