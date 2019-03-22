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


#ifndef PLAYER_INFO_PLAYERINFO_MPV_H
#define PLAYER_INFO_PLAYERINFO_MPV_H

#include "player/info/playerinfo.h"
#include "wzdebug.h"

#include <QList>
#include <QStringList>

class QProcess;

namespace Player {
namespace Info {

class TPlayerInfoMPV : QObject {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TPlayerInfoMPV(const QString& path);

    void getInfo();

    InfoList voList() { return vo_list; }
    InfoList aoList() { return ao_list; }

    InfoList demuxerList() { return demuxer_list; }
    InfoList vcList() { return vc_list; }
    InfoList acList() { return ac_list; }
    QStringList vfList() { return vf_list; }
    QStringList optionList() { return option_list; }

protected:
    QList<QByteArray> run(QString options);
    InfoList getList(const QList<QByteArray> &);
    QStringList getOptionsList(const QList<QByteArray> &);

protected:
    QString bin;

    InfoList vo_list;
    InfoList ao_list;

    InfoList demuxer_list;
    InfoList vc_list;
    InfoList ac_list;

    QStringList vf_list;
    QStringList option_list;
};

} // namespace Info
} // namespace Player

#endif // PLAYER_INFO_PLAYERINFO_MPV_H
