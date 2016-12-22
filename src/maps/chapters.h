/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@escomposlinux.org>

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

#ifndef _MAPS_CHAPTERS_H_
#define _MAPS_CHAPTERS_H_

#include "map.h"

#include <QString>

/* Class to store info about chapters */
namespace Maps {

class TChapterData : public TData {

public:
    TChapterData();
    virtual ~TChapterData() {}

    void setName(const QString & n) { name = n; }
    void setStart(double aStart) { start = aStart; }
    void setEnd(double aEnd) { end = aEnd; }

    QString getName() const { return name; }
    double getStart() const { return start; }
    double getEnd() const { return end; }
    QString getDisplayName() const;

protected:
    QString name;
    double start;
    double end;
};

class TChapters : public TMap<TChapterData> {

public:
    TChapters();
    virtual ~TChapters();

    typedef QMap <int, TChapterData> TChapterMap;
    typedef QMapIterator<int, TChapterData> TChapterIterator;

    void list() const;

    void addName(int id, const QString &name);
    void addStart(int id, double start);
    void addEnd(int id, double end);
    void addChapter(int id, const QString &name, double start);

    int idForTime(double sec, bool allow_gaps = true) const;
};

} // namespace Maps

#endif // _MAPS_CHAPTERS_H_
