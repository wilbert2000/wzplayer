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
#include <QObject>
#include <QList>

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

    InfoList voList() { return vo_list; }
    InfoList aoList() { return ao_list; }
    InfoList demuxerList() { return demuxer_list; }
    InfoList vcList() { return vc_list; }
    InfoList acList() { return ac_list; }

protected slots:
    virtual void readLine(QByteArray);

protected:
    bool run(QString options);

protected:
    QString bin;

    QProcess* proc;

    InfoList vo_list;
    InfoList ao_list;

    InfoList demuxer_list;
    InfoList vc_list;
    InfoList ac_list;

private:
    bool waiting_for_key;
    int reading_type;
};

} // namespace Info
} // namespace Player

#endif // PLAYER_INFO_PLAYERINFOMPLAYER_H
