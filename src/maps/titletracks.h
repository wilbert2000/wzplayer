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

#ifndef MAPS_TITLETRACKS_H
#define MAPS_TITLETRACKS_H

#include <QMap>
#include "map.h"
#include "chapters.h"

/* Class to store info about Cd tracks and DVD titles */

namespace Maps {

class TTitleData : public TData {

public:
    TTitleData();

    TChapters chapters;

    void setName(const QString & aName) { name = aName; }
    void setDuration(double d) { duration = d; }
    void setVTS(int n) { vts = n; }
    void setType(bool aTrack) { isTrack = aTrack; }
    void setTrack(int id, const QString &aName, double aDuration) {
        ID = id;
        name = aName;
        duration = aDuration;
        isTrack = true;
    }

    QString getName() const { return name; }
    double getDuration() const { return duration; }
    int getVTS() const { return vts; }

    QString getDisplayName(bool add_duration = true) const;

protected:
    QString name;
    double duration;
    int vts;
    bool isTrack;
};


class TTitleTracks : public TMap<TTitleData> {

public:
    TTitleTracks();

    typedef QMapIterator<int, TTitleData> TTitleTrackIterator;

    int getSelectedVTS() const { return selectedVTS; }
    void setSelectedVTS(int vts) { selectedVTS = vts; }
    int getVTSCount() const { return vtsCount; }
    void setVTSCount(int count) { vtsCount = count; }
    void setSelectedTitle(int title);
    int findTitleForVTS(int vts);

    void list() const;

    void addName(int ID, const QString &name);
    void addDuration(int ID, double duration, bool is_track = false);
    void addChapters(int ID, int n);
    void addTrack(int ID, const QString &name, double duration);

protected:
    int selectedVTS;
    int vtsCount;
};

} // namespace Maps

#endif // MAPS_TITLETRACKS_H
