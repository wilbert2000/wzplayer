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

#include "player/info/playerinfompv.h"
#include <QStringList>
#include <QProcess>


namespace Player {
namespace Info {

TPlayerInfoMPV::TPlayerInfoMPV(const QString& path)
    : QObject(),
    bin(path) {
}

void TPlayerInfoMPV::getInfo() {

    demuxer_list.clear();
    vc_list.clear();
    ac_list.clear();
    vo_list.clear();
    ao_list.clear();
    vf_list.clear();

    demuxer_list = getList(run("--demuxer help"));
    vc_list = getList(run("--vd help"));
    ac_list = getList(run("--ad help"));
    vo_list = getList(run("--vo help"));
    ao_list = getList(run("--ao help"));

    {
        TNameDescList list = getList(run("--vf help"));
        for (int i = 0; i < list.count(); i++) {
            vf_list.append(list.at(i).name());
        }
    }

    option_list = getOptionsList(run("--list-options"));
}

QList<QByteArray> TPlayerInfoMPV::run(QString options) {
    WZDEBUG("bin '" + bin + "', options '" + options + "'");

    QList<QByteArray> r;

    QStringList args = options.split(" ");
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(bin, args);
    if (!proc.waitForStarted()) {
        WZWARN("process can not start");
        return r;
    }

    //Wait until finish
    if (!proc.waitForFinished()) {
        WZWARN("process did not finish. Killing it...");
        proc.kill();
    }

    QByteArray data = proc.readAll().replace("\r", "");
    r = data.split('\n');
    return r;
}

TNameDescList TPlayerInfoMPV::getList(const QList<QByteArray>& lines) {

    TNameDescList list;

    foreach(QByteArray line, lines) {
        line.replace("\n", "");
        line = line.simplified();
        if (line.startsWith("Available")
                || line.startsWith("demuxer:")
                || line.startsWith("Video decoders:")
                || line.startsWith("Audio decoders:")) {
            line = QByteArray();
        }
        if (!line.isEmpty()) {
            int pos = line.indexOf(' ');
            if (pos >= 0) {
                QString name = line.left(pos);
                if (name.endsWith(':')) name = name.left(name.count() - 1);
                QString desc = line.mid(pos + 1);
                desc = desc.replace(": ", "").replace("- ", "");
                list.append(TNameDesc(name, desc));
                WZTRACE(QString("Added name '%1' desc '%2'")
                        .arg(name).arg(desc));
            }
        }
    }

    return list;
}

QStringList TPlayerInfoMPV::getOptionsList(const QList<QByteArray> & lines) {
    QStringList l;

    foreach(QByteArray line, lines) {
        line.replace("\n", "");
        line = line.simplified();
        if (line.startsWith("--")) {
            int pos = line.indexOf(' ');
            if (pos > -1) {
                QString option_name = line.left(pos);
                l << option_name;
            }
        }
    }

    return l;
}

} // namespace Info
} // namespace Player

#include "moc_playerinfompv.cpp"
