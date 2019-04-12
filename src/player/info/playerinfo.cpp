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

#include "player/info/playerinfo.h"

#include <QFileInfo>
#include <QDateTime>
#include <QSettings>
#include <QDir>

#include "settings/preferences.h"
#include "settings/paths.h"

#include "player/info/playerinfomplayer.h"
#include "player/info/playerinfompv.h"

#define INFOREADER_SAVE_VERSION 3


using namespace Settings;

namespace Player {
namespace Info {

TPlayerInfo* TPlayerInfo::static_obj = 0;

TPlayerInfo* TPlayerInfo::obj() {

    if (!static_obj) {
        static_obj = new TPlayerInfo();
    }
    return static_obj;
}

TPlayerInfo::TPlayerInfo() :
    QObject(),
    bin_size(0) {
}

void TPlayerInfo::getInfo() {
    getInfo(Settings::pref->player_bin);
}

QString TPlayerInfo::getGroup() {
    QString group = bin;
    return group
            .replace("/", "_")
            .replace("\\", "_")
            .replace(".", "_")
            .replace(":", "_");
}

void TPlayerInfo::clearInfo() {

    vo_list.clear();
    ao_list.clear();
    demuxerList().clear();
    vc_list.clear();
    ac_list.clear();
    vf_list.clear();
    option_list.clear();
}

void TPlayerInfo::getInfo(const QString& path) {
    WZDEBUG("'" + path + "'");

    // Player not existing
    QFileInfo fi(path);
    if (!fi.exists()) {
        bin = path;
        bin_size = 0;
        clearInfo();
        WZWARN("Player '" + path + "' not found");
        return;
    }

    // Already loaded info
    qint64 size = fi.size();
    if (path == bin && size == bin_size) {
        WZDEBUG("Reusing player info from memory");
        return;
    }

    bin = path;
    bin_size = size;

    // Get info from ini file
    QString dataPath = TPaths::dataPath();
    if (!QDir().mkpath(dataPath)) {
        WZERROR(QString("Failed to create data directory '%1'. %2")
                .arg(dataPath).arg(strerror(errno)));
    }

    QString inifile = TPaths::playerInfoFileName();
    QSettings set(inifile, QSettings::IniFormat);
    set.beginGroup(getGroup());

    if (set.value("size", -1).toInt() == bin_size) {
        vo_list = convertListToInfoList(set.value("vo_list").toStringList());
        ao_list = convertListToInfoList(set.value("ao_list").toStringList());
        demuxer_list = convertListToInfoList(set.value("demuxer_list").toStringList());
        vc_list = convertListToInfoList(set.value("vc_list").toStringList());
        ac_list = convertListToInfoList(set.value("ac_list").toStringList());
        vf_list = set.value("vf_list").toStringList();
        option_list = set.value("option_list").toStringList();

        WZINFO("Loaded player info from '" + inifile + "'");
        return;
    }

    // Get info from player
    if (TPreferences::getPlayerID(bin) == TPreferences::ID_MPLAYER) {
        TPlayerInfoMplayer ir(bin);
        ir.getInfo();
        vo_list = ir.voList();
        ao_list = ir.aoList();
        demuxer_list = ir.demuxerList();
        vc_list = ir.vcList();
        ac_list = ir.acList();
        vf_list.clear();
        option_list.clear();
    } else {
        TPlayerInfoMPV ir(bin);
        ir.getInfo();
        vo_list = ir.voList();
        ao_list = ir.aoList();
        demuxer_list = ir.demuxerList();
        vc_list = ir.vcList();
        ac_list = ir.acList();
        vf_list = ir.vfList();
        option_list = ir.optionList();
    }

    WZINFO("Saving player info to '" + inifile + "'");
    set.setValue("size", bin_size);
    set.setValue("date", fi.lastModified());
    set.setValue("vo_list", convertInfoListToList(vo_list));
    set.setValue("ao_list", convertInfoListToList(ao_list));
    set.setValue("demuxer_list", convertInfoListToList(demuxer_list));
    set.setValue("vc_list", convertInfoListToList(vc_list));
    set.setValue("ac_list", convertInfoListToList(ac_list));
    set.setValue("vf_list", vf_list);
    set.setValue("option_list", option_list);
    set.sync();
}

QStringList TPlayerInfo::convertInfoListToList(const TNameDescList& l) {

    QStringList r;
    for (int i = 0; i < l.count(); i++) {
        const TNameDesc& d = l.at(i);
        r << d.name() + "|" + d.desc();
    }
    return r;
}

TNameDescList TPlayerInfo::convertListToInfoList(const QStringList& l) {

    TNameDescList r;
    QStringList s;
    for (int n = 0; n < l.count(); n++) {
        s = l.at(n).split("|");
        if (s.count() >= 2) {
            r.append(TNameDesc(s.at(0), s.at(1)));
        }
    }
    return r;
}

} // namespace Info
} // namespace Player

#include "moc_playerinfo.cpp"
