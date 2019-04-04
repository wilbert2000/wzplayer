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


#ifndef PLAYER_INFO_PLAYERINFO_H
#define PLAYER_INFO_PLAYERINFO_H

#include "log4qt/logger.h"
#include <QObject>
#include <QList>
#include <QStringList>


namespace Player {
namespace Info {

class InfoData {
public:
    InfoData() {}
    InfoData(const QString& name, const QString& desc) :
        _name(name),
        _desc(desc) {
    }

    QString name() const { return _name; }
    QString desc() const { return _desc; }

    bool operator<(const InfoData& other) const {
        return name() < other.name();
    }

    bool operator==(const InfoData& other) const {
        return name() == other.name();
    }

private:
    QString _name, _desc;
};


typedef QList<InfoData> InfoList;


class TPlayerInfo : QObject {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TPlayerInfo();

    void getInfo();
    void getInfo(const QString& path);

    InfoList voList() { return vo_list; }
    InfoList aoList() { return ao_list; }

    InfoList demuxerList() { return demuxer_list; }
    InfoList vcList() { return vc_list; }
    InfoList acList() { return ac_list; }
    QStringList vfList() { return vf_list; }
    QStringList optionList() { return option_list; }

    //! Returns an TPlayerInfo object. If it didn't exist before, one
    //! is created.
    static TPlayerInfo* obj();

protected:
    QString bin;
    qint64 bin_size;

    InfoList vo_list;
    InfoList ao_list;
    InfoList demuxer_list;
    InfoList vc_list;
    InfoList ac_list;
    QStringList vf_list;
    QStringList option_list;

private:
    static TPlayerInfo* static_obj;
    static QStringList convertInfoListToList(const InfoList& l);
    static InfoList convertListToInfoList(const QStringList& l);
    QString getGroup();
    void clearInfo();
};

} // namespace Info
} // namespace Player

#endif // PLAYER_INFO_PLAYERINFO_H
