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
#include <QDebug>
#include <QStringList>
#include <QProcess>


namespace Player {
namespace Info {

TPlayerInfoMPV::TPlayerInfoMPV(const QString& path)
    : QObject(),
    debug(logger()),
    bin(path) {
}

TPlayerInfoMPV::~TPlayerInfoMPV() {
}

void TPlayerInfoMPV::getInfo() {

	vo_list.clear();
	ao_list.clear();
	demuxer_list.clear();
	vc_list.clear();
	ac_list.clear();
	vf_list.clear();

	vo_list = getList(run("--vo help"));
	ao_list = getList(run("--ao help"));
	demuxer_list = getList(run("--demuxer help"));
	vc_list = getList(run("--vd help"));
	ac_list = getList(run("--ad help"));
	{
		InfoList list = getList(run("--vf help"));
		for (int n = 0; n < list.count(); n++) {
			vf_list.append(list[n].name());
		}
	}

	option_list = getOptionsList(run("--list-options"));
}

QList<QByteArray> TPlayerInfoMPV::run(QString options) {
    logger()->debug("run: bin '" + bin + "' options '" + options + "'");

	QList<QByteArray> r;

	QStringList args = options.split(" ");
	QProcess proc;
	proc.setProcessChannelMode(QProcess::MergedChannels);
	proc.start(bin, args);
	if (!proc.waitForStarted()) {
        logger()->warn("run: process can't start!");
		return r;
	}

	//Wait until finish
	if (!proc.waitForFinished()) {
        logger()->warn("run: process didn't finish. Killing it...");
		proc.kill();
	}

	QByteArray data = proc.readAll().replace("\r", "");
	r = data.split('\n');
	return r;
}

InfoList TPlayerInfoMPV::getList(const QList<QByteArray> & lines) {
	InfoList l;

	foreach(QByteArray line, lines) {
		line.replace("\n", "");
		line = line.simplified();
		if (line.startsWith("Available") || line.startsWith("demuxer:") ||
            line.startsWith("Video decoders:")
            || line.startsWith("Audio decoders:")) {
			line = QByteArray();
		}
		if (!line.isEmpty()) {
			int pos = line.indexOf(' ');
			if (pos > -1) {
				QString name = line.left(pos);
				if (name.endsWith(':')) name = name.left(name.count()-1);
				QString desc = line.mid(pos+1);
				desc = desc.replace(": ", "").replace("- ", "");
				l.append(InfoData(name, desc));
			}
		}
	}

	return l;
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
