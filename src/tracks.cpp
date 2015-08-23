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

#include "tracks.h"
#include <QRegExp>
#include <QDebug>

Tracks::Tracks() {
	clear();
}

Tracks::~Tracks() {
}

void Tracks::clear() {
	_selected_ID = -1;
	tm.clear();
}

void Tracks::addID(int ID) {
	tm[ID].setID(ID);
}

void Tracks::addName(int ID, const QString &name) {
	tm[ID].setID(ID);
	tm[ID].setName(name);
}

void Tracks::addLang(int ID, QString lang) {
	tm[ID].setID(ID);
	tm[ID].setLang(lang);
}

void Tracks::addTrack(int ID, const QString &lang, const QString &name) {
	tm[ID].setID(ID);
	tm[ID].setLang(lang);
	tm[ID].setName(name);
}

bool Tracks::updateTrack(int id, const QString &type, const QString &value) {

	bool changed = false;
	int idx = find(id);
	if (type == "NAME") {
		if (idx == -1) {
			changed = true;
			addName(id, value);
		} else if (itemAt(idx).name() != value) {
			changed = true;
			addName(id, value);
		}
	} else {
		// LANG
		if (idx == -1) {
			changed = true;
			addLang(id, value);
		} else if (itemAt(idx).lang() != value) {
			changed = true;
			addLang(id, value);
		}
	}

	if (changed) {
		qDebug("Tracks::updateTrack: updated track id %d, type '%s', value '%s'",
			   id, type.toUtf8().constData(), value.toUtf8().constData());

	} else {
		qDebug("Tracks::updateTrack: track id %d already up to date", id);
	}

	return changed;
}


// updateTrack used by MPV
bool Tracks::updateTrack(int ID, const QString &lang, const QString &name, bool selected) {

	bool changed = false;
	TrackMap::iterator i = tm.find(ID);
	if (i == tm.end()) {
		// Not found: add it
		changed = true;
		addTrack(ID, lang, name);
		if (selected)
			_selected_ID = ID;
		qDebug("Tracks::updateTrack: added new track id %d, lang '%s', name '%s', selected %d",
			   ID, lang.toUtf8().constData(), name.toUtf8().constData(), selected);
	} else {
		// Existing track
		if (i.value().lang() != lang) {
			qDebug() << "Tracks::updateTrack: updating lang track id"
					 << ID << "from" << i.value().lang() << "to" << lang;
			i.value().setLang(lang);
			changed = true;
		}
		if (i.value().name() != name) {
			qDebug() << "Tracks::updateTrack: updating name track id"
					 << ID << "from" << i.value().name() << "to" << name;
			i.value().setName(name);
			changed = true;
		}

		if (selected) {
			if (_selected_ID != ID) {
				qDebug() << "Tracks::updateTrack: changed selected id from"
						 << _selected_ID << "to" << ID;
				_selected_ID = ID;
				changed = true;
			}
		} else if (_selected_ID == ID) {
			qDebug() << "Tracks::updateTrack: changed selected id from"
					 << ID << "to -1";
			_selected_ID = -1;
			changed = true;
		}
	}

	if (!changed)
		qDebug("Tracks::updateTrack: track id %d was up to date", ID);

	return changed;
}

int Tracks::numItems() {
	return tm.count();
}

bool Tracks::existsItemAt(int n) {
	return ((n > 0) && (n < numItems()));
}

bool Tracks::existingID(int ID) {

	TrackMap::iterator i = tm.find(ID);
	return i != tm.end();
}

TrackData Tracks::itemAt(int n) {
	return tm.values()[n];
}

TrackData Tracks::item(int ID) {
	return tm[ID];
}

int Tracks::find(int ID) {

	for (int n = 0; n < numItems(); n++) {
		if (itemAt(n).ID() == ID)
			return n;
	}

	return -1;
}

int Tracks::findLang(QString expr) {
	qDebug( "Tracks::findLang: '%s'", expr.toUtf8().data());
	QRegExp rx( expr );

	int res_id = -1;

	for (int n = 0; n < numItems(); n++) {
		qDebug("Tracks::findLang: lang #%d '%s'", n, itemAt(n).lang().toUtf8().data());
		if (rx.indexIn( itemAt(n).lang() ) > -1) {
			qDebug("Tracks::findLang: found preferred lang!");
			res_id = itemAt(n).ID();
			break;	
		}
	}

	return res_id;
}

void Tracks::list() {
	qDebug("Tracks::list: selected track ID %d", _selected_ID);

	QMapIterator<int, TrackData> i(tm);
	while (i.hasNext()) {
		i.next();
		TrackData d = i.value();
		qDebug("Tracks::list: ID: %d lang: '%s' name: '%s'",
			   d.ID(), d.lang().toUtf8().constData(), d.name().toUtf8().constData() );
	}
}

