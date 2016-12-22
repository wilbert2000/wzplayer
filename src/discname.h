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

#ifndef DISCNAME_H
#define DISCNAME_H

#include <QString>


class TDiscName {
public:
    enum TDisc { DVD = 1, DVDNAV = 2, VCD = 3, CDDA = 4, BLURAY = 5 };

    TDiscName();
    TDiscName(const TDiscName& disc);
    TDiscName(const QString& aprotocol, int atitle, const QString& adevice);
    TDiscName(const QString& adevice, bool use_dvd_nav);
    TDiscName(const QString& url);
    virtual ~TDiscName();

    static TDisc protocolToTDisc(QString protocol);

    QString toString(bool add_zero_title = false) const;
    TDisc disc() const;

    QString displayName(bool addDevice = true) const;

    QString protocol;
    int title;
    QString device;
    bool valid;

private:
    void removeTrailingSlashFromDevice();
};

#endif // DISCNAME_H
