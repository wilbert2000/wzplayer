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


#ifndef _SUBTRACKS_H_
#define _SUBTRACKS_H_

#include <QString>
#include <QFileInfo>
#include <QList>

class SubData {

public:
	enum Type { None = -1, File = 0, Vob = 1, Sub = 2};

	SubData();
	SubData(Type type,
			int id,
			const QString& lang,
			const QString& name,
			const QString& filename);
	virtual ~SubData();

	void setType(Type t) { _type = t; }
	void setID(int id) { _ID = id; }
	void setLang(QString lang) { _lang = lang; }
	void setName(QString name) { _name = name; }
	void setFilename(QString f) { _filename = f; }

	Type type() const { return _type; }
	int ID() const { return _ID; }
	QString lang() const { return _lang; }
	QString name() const { return _name; }
	QString filename() const { return _filename; }

	QString displayName() const;

protected:
	Type _type;
	int _ID;
	QString _lang;
	QString _name;
	QString _filename;
};

typedef QListIterator<SubData> SubIterator;

class SubTracks {
public:
	SubTracks();
	virtual ~SubTracks();

	void clear();

	SubData::Type selectedType() const { return _selected_type; }
	int selectedID() const { return _selected_ID; }
	void setSelected(SubData::Type type, int id) {
		_selected_type = type;
		_selected_ID = id;
	}
	void clearSelected() { _selected_ID = -1; }

#ifdef MPV_SUPPORT
	SubData::Type selectedSecondaryType() const { return _selected_secondary_type; }
	int selectedSecondaryID() const { return _selected_secondary_ID; }
	void setSelectedSecondary(SubData::Type type, int id) {
		_selected_secondary_type = type;
		_selected_secondary_ID = id;
	}
#endif

	int count() const { return subs.count(); }

	int find(SubData::Type type, int ID) const;
	int findSelectedIdx() const;

#ifdef MPV_SUPPORT
	int findSelectedSecondaryIdx() const;
#endif

	int findLangIdx(QString expr) const;
	SubData findItem(SubData::Type t, int ID) const;
	SubData itemAt(int n) const;

	int firstID() const;
	int nextID() const;
	bool hasFileSubs() const;

	int selectOne(QString preferred_lang, int default_sub = 0) const;

	void add(SubData::Type t, int ID);
	bool changeLang(SubData::Type t, int ID, QString lang);
	bool changeName(SubData::Type t, int ID, QString name);
	bool changeFilename(SubData::Type t, int ID, QString filename);

#ifdef MPV_SUPPORT
	bool update(SubData::Type type, int id, SubData::Type sec_type, int sec_id, const QString & lang, const QString & name, const QString &filename, bool selected, bool sec_selected);
#endif

	void list() const;
	void listNames() const;

protected:
	typedef QList <SubData> SubList;
	SubList subs;
	SubData::Type _selected_type;
	int _selected_ID;

#ifdef MPV_SUPPORT
	SubData::Type _selected_secondary_type;
	int _selected_secondary_ID;
#endif
};

#endif
