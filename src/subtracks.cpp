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


#include "subtracks.h"
#include "mediasettings.h"
#include <QRegExp>
#include <QDebug>

SubTracks::SubTracks() {
	_selected_type = SubData::Sub;
	_selected_ID = -1;
}


SubTracks::~SubTracks() {
}

void SubTracks::clear() {
	// Don't clear type: for mpv it's always sub
	_selected_ID = -1;
	subs.clear();
}

int SubTracks::numItems() {
	return subs.count();
}

bool SubTracks::existsItemAt(int n) {
	return ((n > 0) && (n < numItems()));
}

int SubTracks::find( SubData::Type t, int ID ) {
	for (int n = 0; n < subs.count(); n++) {
		if ( ( subs[n].type() == t ) && ( subs[n].ID() == ID ) ) {
			return n;
		}
	}
	//qDebug("SubTracks::find: item type: %d, ID: %d doesn't exist", t, ID);
	return -1;
}

int SubTracks::findLang(QString expr) {
	qDebug( "SubTracks::findLang: '%s'", expr.toUtf8().data());
	QRegExp rx( expr );

	int res_id = -1;

	for (int n = 0; n < numItems(); n++) {
		qDebug("SubTracks::findLang: lang #%d '%s'", n, 
                subs[n].lang().toUtf8().data());
		if (rx.indexIn( subs[n].lang() ) > -1) {
			qDebug("SubTracks::findLang: found preferred lang!");
			res_id = n;
			break;	
		}
	}

	return res_id;
}

int SubTracks::findFile(const QString &filename, int not_found_idx) {

	int result = not_found_idx;
	bool found = false;

	for (int n = 0; n < numItems(); n++) {
		SubData sub = subs[n];
		if (sub.type() == SubData::File && sub.filename() == filename) {
			result = n;
			found = true;
			break;
		}
	}

	if (found)
		qDebug() << "SubTracks::findFile: found" << filename;
	else qDebug() << "SubTracks::findFile:" << filename << "not found";

	return result;
}

SubData SubTracks::findItem( SubData::Type t, int ID ) {
	SubData sub;
	int n = find(t,ID);
	if ( n != -1 )
		return subs[n];
	else
		return sub;
}

SubData SubTracks::itemAt( int n ) {
	if (n >= 0 && n < subs.count()) {
		return subs[n];
	} else {
		qWarning("SubTracks::itemAt: %d out of range!", n);
		qWarning("SubTracks::itemAt: returning an empty sub to avoid a crash");
		qWarning("SubTracks::itemAt: this shouldn't happen, report a bug if you see this");

		SubData empty_sub;
		return empty_sub;
	}
}

// Return first subtitle or the user preferred (if found)
// or none if there's no subtitles
int SubTracks::selectOne(QString preferred_lang, int default_sub) {

	int sub = MediaSettings::SubNone;

	if (numItems() > 0) {
		sub = 0; // First subtitle
		if (existsItemAt(default_sub)) {
			sub = default_sub;
		}

		// Check if one of the subtitles is the user preferred.
		if (!preferred_lang.isEmpty()) {
			int res = findLang( preferred_lang );
			if (res != -1) sub = res;
		}
	}

	return sub;
}

void SubTracks::add( SubData::Type t, int ID ) {
	SubData d;
	d.setType(t);
	d.setID(ID);

	subs.append(d);
}

bool SubTracks::changeLang( SubData::Type t, int ID, QString lang ) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setLang(lang);
	return true;
}

bool SubTracks::changeName( SubData::Type t, int ID, QString name ) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setName(name);
	return true;
}

bool SubTracks::changeFilename( SubData::Type t, int ID, QString filename ) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setFilename(filename);
	return true;
}

bool SubTracks::update(int id, const QString & lang, const QString & name, bool selected) {

	bool changed = false;

	int idx = find(SubData::Sub, id);
	if (idx == -1) {
		changed = true;
		add(SubData::Sub, id);
		changeName(SubData::Sub, id, name);
		changeLang(SubData::Sub, id, lang);
		if (selected)
			_selected_ID = id;
	} else {
		// Track already existed
		if (itemAt(idx).name() != name) {
			changed = true;
			changeName(SubData::Sub, id, name);
		}
		if (itemAt(idx).lang() != lang) {
			changed = true;
			changeLang(SubData::Sub, id, lang);
		}

		if (selected) {
			if (_selected_ID != id) {
				qDebug() << "SubTracks::update: changed selected id from"
						 << _selected_ID << "to" << id;
				_selected_ID = id;
				changed = true;
			}
		} else if (_selected_ID == id) {
			qDebug() << "SubTracks::update: changed selected id from"
					 << id << "to -1";
			_selected_ID = -1;
			changed = true;
		}
	}

	if (changed) {
		qDebug("SubTracks::update: updated subtitle track id: %d, lang: '%s', name: '%s', selected: %d",
			   id, lang.toUtf8().constData(), name.toUtf8().constData(), selected);
	} else {
		qDebug("SubTracks::update:: subtitle track id %d already up to date", id);
	}
	return changed;
}

void SubTracks::list() {
	qDebug("SubTracks::list: selected subtitle track ID: %d", _selected_ID);
	for (int n = 0; n < subs.count(); n++) {
		qDebug("SubTracks::list: item %d: type: %d ID: %d lang: '%s' name: '%s' filename: '%s'",
			   n, subs[n].type(), subs[n].ID(), subs[n].lang().toUtf8().data(),
			   subs[n].name().toUtf8().data(), subs[n].filename().toUtf8().data() );
	}
}

void SubTracks::listNames() {
	for (int n = 0; n < subs.count(); n++) {
		qDebug("SubTracks::list: item %d: '%s'",
			   n, subs[n].displayName().toUtf8().data() );
	}
}

/*
void SubTracks::test() {
	process("ID_SUBTITLE_ID=0");
	process("ID_SID_0_NAME=Arabic");
	process("ID_SID_0_LANG=ara");
	process("ID_SUBTITLE_ID=1");
	process("ID_SID_1_NAME=Catalan");
	process("ID_SID_1_LANG=cat");

	process("ID_VOBSUB_ID=0");
	process("ID_VSID_0_LANG=en");
	process("ID_VOBSUB_ID=1");
	process("ID_VSID_1_LANG=fr");

	process("ID_FILE_SUB_ID=1");
	process("ID_FILE_SUB_FILENAME=./lost313_es.sub");

	list();
	listNames();
}
*/
