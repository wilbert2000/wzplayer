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
#include "settings/mediasettings.h"
#include <QRegExp>
#include <QDebug>

SubData::SubData()
	: _type(None)
	, _ID(-1) {
}

SubData::SubData(Type type,
				 int id,
				 const QString& lang,
				 const QString& name,
				 const QString& filename)
	: _type(type)
	, _ID(id)
	, _lang(lang)
	, _name(name)
	, _filename(filename) {
}

SubData::~SubData() {
}

// TODO: shorten if too long
QString SubData::displayName() const {

	QString dname = "";

	if (!_name.isEmpty()) {
		dname = _name;
		if (!_lang.isEmpty()) {
			dname += " ["+ _lang + "]";
		}
	} else if (!_lang.isEmpty()) {
		dname = _lang;
	} else if (!_filename.isEmpty()) {
		QFileInfo f(_filename);
		dname = f.fileName();
	} else dname = QString::number(_ID);

	return dname;
}

SubTracks::SubTracks()
	: _selected_type(SubData::None)
	, _selected_ID(-1)

#ifdef MPV_SUPPORT
	, _selected_secondary_type(SubData::None)
	,  _selected_secondary_ID(-1)
#endif
{}

SubTracks::~SubTracks() {
}

void SubTracks::clear() {
	_selected_type = SubData::None;
	_selected_ID = -1;

#ifdef MPV_SUPPORT
	_selected_secondary_type = SubData::None;
	_selected_secondary_ID = -1;
#endif

	subs.clear();
}

int SubTracks::find(SubData::Type type, int ID) const {

	for (int n = 0; n < subs.count(); n++) {
		if ((subs[n].type() == type) && (subs[n].ID() == ID)) {
			return n;
		}
	}

	return -1;
}

int SubTracks::findSelectedIdx() const {
	return find(_selected_type, _selected_ID);
}

#ifdef MPV_SUPPORT
int SubTracks::findSelectedSecondaryIdx() const {
	return find(_selected_secondary_type, _selected_secondary_ID);
}
#endif

int SubTracks::findLangIdx(QString expr) const {
	qDebug("SubTracks::findLangIdx: '%s'", expr.toUtf8().data());

	QRegExp rx(expr);

	for (int n = 0; n < subs.count(); n++) {
		if (rx.indexIn(subs[n].lang()) >= 0) {
			qDebug("SubTracks::findLangIdx: found preferred language");
			return n;
		}
	}

	qDebug("SubTracks::findLangIdx: no match for preferred language found");
	return -1;
}

SubData SubTracks::findItem(SubData::Type t, int ID) const {

	int n = find(t, ID);
	if (n >= 0)
		return subs[n];

	return SubData();
}

int SubTracks::firstID() const {

	if (subs.count() > 0) {
		return subs.at(0).ID();
	}
	return -1;
}

int SubTracks::nextID() const {

	int idx = findSelectedIdx();
	if (idx < 0) {
		if (subs.count() > 0)
			return 0;
		return -1;
	}
	idx++;
	if (idx >= subs.count()) {
		return -1;
	}
	return idx;
}

bool SubTracks::hasFileSubs() const {

	for (int n = 0; n < subs.count(); n++) {
		if (subs[n].type() == SubData::File) {
			return true;
		}
	}
	return false;
}

SubData SubTracks::itemAt(int n) const {

	if (n >= 0 && n < subs.count())
		return subs[n];
	return SubData();
}

// Return first subtitle idx or the user preferred (if found)
// or SubNone if there are no subtitles
int SubTracks::selectOne(QString preferred_lang, int default_sub) const {

	int sub = Settings::TMediaSettings::SubNone;

	if (subs.count() > 0) {
		// First subtitle
		sub = 0;
		// Default
		if (default_sub > 0 && default_sub < subs.count()) {
			sub = default_sub;
		}
		// Check language
		if (!preferred_lang.isEmpty()) {
			int idx = findLangIdx(preferred_lang);
			if (idx != -1)
				sub = idx;
		}
	}

	return sub;
}

void SubTracks::add(SubData::Type t, int ID) {
	SubData d;
	d.setType(t);
	d.setID(ID);

	subs.append(d);
}

bool SubTracks::changeLang(SubData::Type t, int ID, QString lang) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setLang(lang);
	return true;
}

bool SubTracks::changeName(SubData::Type t, int ID, QString name) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setName(name);
	return true;
}

bool SubTracks::changeFilename(SubData::Type t, int ID, QString filename) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setFilename(filename);
	return true;
}

#ifdef MPV_SUPPORT
bool SubTracks::update(SubData::Type type,
					   int id,
					   SubData::Type sec_type,
					   int sec_id,
					   const QString & lang,
					   const QString & name,
					   const QString & filename,
					   bool selected,
					   bool sec_selected) {

	bool changed = false;

	int idx = find(type, id);
	if (idx < 0) {
		changed = true;
		SubData sub(type, id, lang, name, filename);
		subs.append(sub);

		if (selected) {
			_selected_type = type;
			_selected_ID = id;
		}
		if (sec_selected) {
			_selected_secondary_type = sec_type;
			_selected_secondary_ID = sec_id;
		}
	} else {
		// Track already existed, so type and id match
		SubData &sub = subs[idx];
		if (sub.lang() != lang) {
			changed = true;
			sub.setLang(lang);
		}
		if (sub.name() != name) {
			changed = true;
			sub.setName(name);
		}
		if (sub.filename() != filename) {
			changed = true;
			sub.setFilename(filename);
		}

		if (selected) {
			if (_selected_type != type || _selected_ID != id) {
				qDebug() << "SubTracks::update: changed selected subtitle from"
						 << _selected_type << _selected_ID
						 << "to" << type << id;
				_selected_type = type;
				_selected_ID = id;
				changed = true;
			}
		} else if (_selected_type == type && _selected_ID == id) {
			qDebug() << "SubTracks::update: changed selected subtitle from"
					 << type << id << "to none selected";
			_selected_type = SubData::None;
			_selected_ID = -1;
			changed = true;
		}

		// Secondary subtitle
		if (sec_selected) {
			if (_selected_secondary_type != sec_type
				|| _selected_secondary_ID != sec_id) {
				qDebug() << "SubTracks::update: changed selected secondary subtitle from"
						 << _selected_secondary_type << _selected_secondary_ID
						 << "to" << sec_type << sec_id;
				_selected_secondary_type = sec_type;
				_selected_secondary_ID = sec_id;
				changed = true;
			}
		} else if (sec_id >= 0
				   && _selected_secondary_type == sec_type
				   && _selected_secondary_ID == sec_id) {
			qDebug() << "SubTracks::update: changed selected secondary subtitle from"
					 << sec_type << sec_id << "to none selected";
			_selected_secondary_type = SubData::None;
			_selected_secondary_ID = -1;
			changed = true;
		}

	}

	if (changed) {
		qDebug() << "SubTracks::update: updated subtitle track type:" << type
				 << "id:" << id
				 << "sec type:" << sec_type
				 << "sec id:" << sec_id
				 << "lang:" << lang
				 << "name:" << name
				 << "filename:" << filename
				 << "selected:" << selected
				 << "sec selected:" << sec_selected;
	} else {
		qDebug("SubTracks::update:: subtitle track type %d id %d was up to date", type, id);
	}
	return changed;
}
#endif

void SubTracks::list() const {
	qDebug("SubTracks::list: selected subtitle track ID: %d", _selected_ID);
	for (int n = 0; n < subs.count(); n++) {
		qDebug("SubTracks::list: item %d: type: %d ID: %d lang: '%s' name: '%s' filename: '%s'",
			   n, subs[n].type(), subs[n].ID(), subs[n].lang().toUtf8().data(),
			   subs[n].name().toUtf8().data(), subs[n].filename().toUtf8().data());
	}
}

void SubTracks::listNames() const {
	for (int n = 0; n < subs.count(); n++) {
		qDebug("SubTracks::list: item %d: '%s'",
			   n, subs[n].displayName().toUtf8().data());
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
