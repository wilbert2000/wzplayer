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

#ifndef _TRACKS_H_
#define _TRACKS_H_

#include <QString>
#include <QMap>

/* Class to store info about video/audio tracks */

class TrackData {

public:

	TrackData() { _lang = ""; _name = "";_ID = -1; };
	~TrackData() {};

	void setID( int id ) { _ID = id; };
	void setLang( const QString & l ) { _lang = l; };
	void setName( const QString & n ) { _name = n; };

	int ID() const { return _ID; };
	QString lang() const { return _lang; };
	QString name() const { return _name; };

	QString displayName() const {
		QString dname="";

	    if (!_name.isEmpty()) {
    	    dname = _name;
			if (!_lang.isEmpty()) {
				dname += " ["+ _lang + "]";
			}
		}
	    else
	    if (!_lang.isEmpty()) {
	        dname = _lang;
		}
	    else
	    dname = QString::number(_ID);

		return dname;
	}

protected:
	int _ID;

	/* Language code: es, en, etc. */
	QString _lang;

	/* spanish, english... */
	QString _name;
};

class Tracks {

public:

	Tracks();
	~Tracks();

	void clear();
	void list();

	void addID(int ID);
	void addLang(int ID, QString lang);
	void addName(int ID, const QString &name);
	void addTrack(int ID, const QString &lang, const QString &name);
	// For mplayer
	bool updateTrack(int id, const QString &type, const QString &value);
	// For mpv
	bool updateTrack(int ID, const QString &lang, const QString &name, bool selected);

	int numItems();
	bool existsItemAt(int n);
	bool existingID(int ID);


	int selectedID() { return _selected_ID; }
	void setSelectedID(int ID) { _selected_ID = ID; }

	TrackData itemAt(int n);
	TrackData item(int ID);

	int find(int ID);
	int findLang(QString expr);

protected:
	typedef QMap <int, TrackData> TrackMap;
	TrackMap tm;
	int _selected_ID;
};

#endif
