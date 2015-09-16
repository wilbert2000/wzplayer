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

#ifndef _MAPS_TRACKS_H_
#define _MAPS_TRACKS_H_

#include <QString>
#include "map.h"

/* Class to store info about video/audio tracks */

namespace Maps {

class TTrackData : public TData {
public:
	TTrackData() {}
	~TTrackData() {}

	QString getLang() const { return lang; }
	QString getName() const { return name; }

	void setLang( const QString& aLang ) { lang = aLang; }
	void setName( const QString& aName ) { name = aName; }

	void set(int id, const QString &aLang, const QString &aName) {
		ID = id;
		lang = aLang;
		name = aName;
	}


	QString getDisplayName() const;

protected:

	/* Language code: es, en, etc. */
	QString lang;

	/* spanish, english... */
	QString name;
};

class TTracks : public TMap<TTrackData> {

public:

	TTracks() {}
	~TTracks() {}

	typedef QMapIterator<int, TTrackData> TTrackIterator;

	TTrackIterator getIterator() const { return TTrackIterator(*this); }

	void list() const;

	void addLang(int id, const QString &lang);
	void addName(int id, const QString &name);
	void addTrack(int id, const QString &lang, const QString &name);

	// For mplayer
	bool updateTrack(int id, const QString &field, const QString &value);
	// For both
	bool updateTrack(int ID, const QString &lang, const QString &name, bool selected);

	// Select a track matching expr if only one track matches
	int findLangID(QString expr) const;
};

} // namespace Maps

#endif // _MAPS_TRACKS_H_
