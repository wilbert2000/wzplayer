/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@escomposlinux.org>

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

#include "chapters.h"
#include "helper.h"

namespace Maps {

TChapterData::TChapterData() :
	TData(),
	name(),
	start(-1),
	end(-1) {
}

QString TChapterData::getDisplayName() const {

	QString dname = QString::number(ID + 1);

	if (!name.isEmpty()) {
		dname += " " + name;
	}

	if (start >= 0) {
		dname += " (" + Helper::formatTime(qRound(start)) + ")";
	}

	return dname;
};

void TChapters::addName(int id, const QString &name) {
	TChapterData& chapter = (*this)[id];
	chapter.setID(id);
	chapter.setName(name);
}

void TChapters::addStart(int id, double start) {
	TChapterData& chapter = (*this)[id];
	chapter.setID(id);
	chapter.setStart(start);
}

void TChapters::addEnd(int id, double end) {
	TChapterData& chapter = (*this)[id];
	chapter.setID(id);
	chapter.setEnd(end);
}

void TChapters::addChapter(int id, const QString &name, double start) {
	TChapterData& chapter = (*this)[id];
	chapter.setID(id);
	chapter.setName(name);
	chapter.setStart(start);
}

int TChapters::idForTime(double sec, bool allow_gaps) const {

	int id = -1;

	TChapterIterator i(*this);
	while(i.hasNext()) {
		i.next();
		const TChapterData chapter = i.value();
		if(sec < chapter.getStart()) {
			// return previous id
			return id;
		}
		id = chapter.getID();
		if (sec < chapter.getEnd()) {
			// return current id
			return id;
		}
		// if chapter has end set reset id
		if (allow_gaps && chapter.getEnd() != -1) {
			id = -1;
		}
	}

	return id;
}

void TChapters::list() const {
	qDebug("Maps::TChapters::list: selected ID: %d", selectedID);
	TChapterIterator i(*this);
	while (i.hasNext()) {
		i.next();
		const TChapterData d = i.value();
		qDebug("Maps::TChapters::list: ID: %d name: '%s' start: %g end: %g",
			   d.getID(), d.getName().toUtf8().constData(),
			   d.getStart(), d.getEnd());
	}
}

} // End namespace Maps
