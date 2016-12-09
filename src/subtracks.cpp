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


#include "subtracks.h"
#include "settings/mediasettings.h"
#include <QRegExp>
#include "wzdebug.h"


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


LOG4QT_DECLARE_STATIC_LOGGER(logger, TSubTracks)


TSubTracks::TSubTracks()
	: _selected_type(SubData::None)
	, _selected_ID(-1)
	, _selected_secondary_type(SubData::None)
	,  _selected_secondary_ID(-1)
{}

TSubTracks::~TSubTracks() {
}

void TSubTracks::clear() {
	_selected_type = SubData::None;
	_selected_ID = -1;

	_selected_secondary_type = SubData::None;
	_selected_secondary_ID = -1;

	subs.clear();
}

int TSubTracks::find(SubData::Type type, int ID) const {

	for (int n = 0; n < subs.count(); n++) {
		if ((subs[n].type() == type) && (subs[n].ID() == ID)) {
			return n;
		}
	}

	return -1;
}

int TSubTracks::findSelectedIdx() const {
	return find(_selected_type, _selected_ID);
}

int TSubTracks::findSelectedSecondaryIdx() const {
	return find(_selected_secondary_type, _selected_secondary_ID);
}

int TSubTracks::findLangIdx(QString expr) const {
    WZDEBUG("'" + expr + "'");

	QRegExp rx(expr);

	for (int n = 0; n < subs.count(); n++) {
		if (rx.indexIn(subs[n].lang()) >= 0) {
            WZDEBUG("found preferred language");
			return n;
		}
	}

    WZDEBUG("no match for preferred language found");
	return -1;
}

SubData TSubTracks::findItem(SubData::Type t, int ID) const {

	int n = find(t, ID);
	if (n >= 0)
		return subs[n];

	return SubData();
}

int TSubTracks::firstID() const {

	if (subs.count() > 0) {
		return subs.at(0).ID();
	}
	return -1;
}

int TSubTracks::nextID() const {

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

bool TSubTracks::hasFileSubs() const {

	for (int n = 0; n < subs.count(); n++) {
		if (subs[n].type() == SubData::File) {
			return true;
		}
	}
	return false;
}

SubData TSubTracks::itemAt(int n) const {

	if (n >= 0 && n < subs.count())
		return subs[n];
	return SubData();
}

void TSubTracks::add(SubData::Type t, int ID) {
	SubData d;
	d.setType(t);
	d.setID(ID);

	subs.append(d);
}

bool TSubTracks::changeLang(SubData::Type t, int ID, QString lang) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setLang(lang);
	return true;
}

bool TSubTracks::changeName(SubData::Type t, int ID, QString name) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setName(name);
	return true;
}

bool TSubTracks::changeFilename(SubData::Type t, int ID, QString filename) {
	int f = find(t,ID);
	if (f == -1) return false;

	subs[f].setFilename(filename);
	return true;
}

bool TSubTracks::update(SubData::Type type,
					   int id,
					   SubData::Type sec_type,
					   int sec_id,
                       const QString& lang,
                       const QString& name,
                       const QString& filename,
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
                WZDEBUG("changed selected subtitle from type "
                    + QString::number(_selected_secondary_type)
                    + " id " + QString::number(_selected_ID)
                    + " to type " + QString::number(type)
                    + " id " + QString::number(id));
				_selected_type = type;
				_selected_ID = id;
				changed = true;
			}
		} else if (_selected_type == type && _selected_ID == id) {
            WZDEBUG("changed selected subtitle from " + QString::number(type)
                    + " " + QString::number(id) + " to none selected");
			_selected_type = SubData::None;
			_selected_ID = -1;
			changed = true;
		}

		// Secondary subtitle
		if (sec_selected) {
			if (_selected_secondary_type != sec_type
				|| _selected_secondary_ID != sec_id) {
                WZDEBUG("changed selected secondary subtitle from type "
                        + QString::number(_selected_secondary_type)
                        + " id " + QString::number(_selected_secondary_ID)
                        + " to type " + QString::number(sec_type)
                        + " id " + QString::number(sec_id));
                _selected_secondary_type = sec_type;
				_selected_secondary_ID = sec_id;
				changed = true;
			}
		} else if (sec_id >= 0
				   && _selected_secondary_type == sec_type
				   && _selected_secondary_ID == sec_id) {
            WZDEBUG("changed selected secondary subtitle from type "
                    + QString::number(sec_type) + " id "
                    + QString::number(sec_id) + " to none selected");
			_selected_secondary_type = SubData::None;
			_selected_secondary_ID = -1;
			changed = true;
		}

	}

	if (changed) {
        WZDEBUG("updated subtitle track type: " + QString::number(type)
                + " id: " + QString::number(id)
                + " sec type: " + QString::number(sec_type)
                + " sec id: " + QString::number(sec_id)
                + " lang: '" + lang
                + "' name: '" + name
                + "' filename: '" + filename
                + "' selected: " + QString::number(selected)
                + " sec selected: " + QString::number(sec_selected));
	} else {
        WZDEBUG("subtitle track type " + QString::number(type) + " id "
                + QString::number(id) + " was up to date");
	}
	return changed;
}

void TSubTracks::list() const {
    WZDEBUG("selected subtitle track ID: " + QString::number(_selected_ID));

    int n = 0;
    foreach(const SubData sub, subs) {
        n++;
        WZDEBUG("item " + QString::number(n)
                + " type: " + QString::number(sub.type())
                + " ID: " + QString::number(sub.ID())
                + " lang: '" + sub.lang() + "'"
                + " name: '" + sub.name() + "'"
                + " filename: '" + sub.filename() + "'");
	}
}

