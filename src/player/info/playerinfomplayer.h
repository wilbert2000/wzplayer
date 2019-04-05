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


#ifndef PLAYER_INFO_PLAYERINFOMPLAYER_H
#define PLAYER_INFO_PLAYERINFOMPLAYER_H

#include "player/info/playerinfo.h"
#include "wzdebug.h"

class QProcess;

namespace Player {
namespace Info {

class TPlayerInfoMplayer : QObject {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER
public:
    TPlayerInfoMplayer(const QString& path);

    void getInfo();

    TNameDescList demuxerList() { return demuxer_list; }
    TNameDescList voList() { return vo_list; }
    TNameDescList vcList() { return vc_list; }
    TNameDescList acList() { return ac_list; }
    TNameDescList aoList() { return ao_list; }

protected slots:
    virtual void readLine(QByteArray);

protected:
    bool run(QString options);

protected:
    QString bin;
    QProcess* proc;

    TNameDescList demuxer_list;
    TNameDescList vc_list;
    TNameDescList ac_list;
    TNameDescList vo_list;
    TNameDescList ao_list;

private:
    bool waiting_for_key;
    int reading_type;
};

} // namespace Info
} // namespace Player

#endif // PLAYER_INFO_PLAYERINFOMPLAYER_H
