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

#ifndef _MAPS_TITLETRACKS_H_
#define _MAPS_TITLETRACKS_H_

#include <QMap>
#include "helper.h"
#include "map.h"
#include "chapters.h"

/* Class to store info about Cd tracks and DVD titles */

namespace Maps {

class TTitleData : public TData {

public:
	TTitleData();
	virtual ~TTitleData() {}

	TChapters chapters;

	void setName( const QString & aName ) { name = aName; }
	void setDuration( double d ) { duration = d; }
	void setAngles( int n ) { angles = n; }
	void setVTS( int n ) { vts = n; }
	void setType( bool aTrack ) { isTrack = aTrack; }
	void setTrack(int id, const QString &aName, double aDuration) {
		ID = id;
		name = aName;
		duration = aDuration;
		isTrack = true;
	}

	QString getName() const { return name; }
	double getDuration() const { return duration; }
	int getAngles() const { return angles; }
	int getVTS() const { return vts; }

	QString getDisplayName(bool add_duration = true) const;

protected:
	QString name;
	double duration;
	int angles;
	int vts;
	bool isTrack;
};


class TTitleTracks : public TMap<TTitleData> {

public:
	TTitleTracks() : selectedVTS(-1), vtsCount(0) {}
	virtual ~TTitleTracks() {}

	typedef QMapIterator<int, TTitleData> TTitleTrackIterator;

	int getSelectedVTS() const { return selectedVTS; }
	void setSelectedVTS(int vts) { selectedVTS = vts; }
	int getVTSCount() const { return vtsCount; }
	void setVTSCount(int count) { vtsCount = count; }
	bool setTitleFromDuration(double duration, int titleHint);

	void list() const;

	void addName(int ID, const QString &name);
	void addDuration(int ID, double duration, bool is_track = false);
	void addChapters(int ID, int n);
	void addAngles(int ID, int n);
	void addTrack(int ID, const QString &name, double duration);

protected:
	int selectedVTS;
	int vtsCount;

	void setVTSTitle(int title);
};

} // namespace Maps

#endif // _MAPS_TITLETRACKS_H_
