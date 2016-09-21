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

#include "player/info/playerinfomplayer.h"

#include <QStringList>
#include <QApplication>
#include <QRegExp>
#include <QProcess>

#include "colorutils.h"

#define NOME 0
#define VO 1
#define AO 2
#define DEMUXER 3
#define VC 4
#define AC 5


namespace Player {
namespace Info {

TPlayerInfoMplayer::TPlayerInfoMplayer(const QString& path)
    : QObject(),
    debug(logger()),
    bin(path) {

    proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
}

TPlayerInfoMplayer::~TPlayerInfoMplayer() {
}

void TPlayerInfoMplayer::getInfo() {
	waiting_for_key = true;
	vo_list.clear();
	ao_list.clear();
	demuxer_list.clear();

	run("-identify -vo help -ao help -demuxer help -vc help -ac help");
}

static QRegExp rx_vo_key("^ID_VIDEO_OUTPUTS");
static QRegExp rx_ao_key("^ID_AUDIO_OUTPUTS");

static QRegExp rx_demuxer_key("^ID_DEMUXERS");
static QRegExp rx_ac_key("^ID_AUDIO_CODECS");
static QRegExp rx_vc_key("^ID_VIDEO_CODECS");

static QRegExp rx_driver("\\t(.*)\\t(.*)");
static QRegExp rx_demuxer("^\\s+([A-Z,a-z,0-9]+)\\s+(\\d+)\\s+(\\S.*)");
static QRegExp rx_demuxer2("^\\s+([A-Z,a-z,0-9]+)\\s+(\\S.*)");
static QRegExp rx_codec("^([A-Z,a-z,0-9]+)\\s+([A-Z,a-z,0-9]+)\\s+([A-Z,a-z,0-9]+)\\s+(\\S.*)");

void TPlayerInfoMplayer::readLine(QByteArray ba) {

#if COLOR_OUTPUT_SUPPORT
    QString line = ColorUtils::stripColorsTags(QString::fromLocal8Bit(ba));
#else
	QString line = QString::fromLocal8Bit(ba);
#endif

    if (line.isEmpty())
        return;

	if (!waiting_for_key) {
		if ((reading_type == VO) || (reading_type == AO)) {
            if (rx_driver.indexIn(line) >= 0) {
				QString name = rx_driver.cap(1);
				QString desc = rx_driver.cap(2);
                logger()->debug("readLine: found driver: '" + name
                                + "' '" + desc + "'");
				if (reading_type==VO) {
					vo_list.append(InfoData(name, desc));
				} 
				else
				if (reading_type==AO) {
					ao_list.append(InfoData(name, desc));
				}
			} else {
                logger()->debug("readLine: skipping line: '" + line + "'");
			}
		}
		else
		if (reading_type == DEMUXER) {
            if (rx_demuxer.indexIn(line) >= 0) {
				QString name = rx_demuxer.cap(1);
				QString desc = rx_demuxer.cap(3);
                logger()->debug("readLine: found demuxer: '" + name
                                + "' '" + desc + "'");
				demuxer_list.append(InfoData(name, desc));
			}
			else 
            if (rx_demuxer2.indexIn(line) >= 0) {
				QString name = rx_demuxer2.cap(1);
				QString desc = rx_demuxer2.cap(2);
                logger()->debug("readLine: found demuxer: '" + name
                                + "' '" + desc + "'");
				demuxer_list.append(InfoData(name, desc));
			}
			else {
                logger()->debug("readLine: skipping line: '" + line + "'");
			}
		}
		else
		if ((reading_type == VC) || (reading_type == AC)) {
            if (rx_codec.indexIn(line) >= 0) {
				QString name = rx_codec.cap(1);
				QString desc = rx_codec.cap(4);
                logger()->debug("readLine: found codec: '"
                                + name + "' '" + desc + "'");
				if (reading_type==VC) {
					vc_list.append(InfoData(name, desc));
				} 
				else
				if (reading_type==AC) {
					ac_list.append(InfoData(name, desc));
				}
			} else {
                logger()->debug("readLine: skipping line '" + line + "'");
			}
		}
	}

    if (rx_vo_key.indexIn(line) >= 0) {
		reading_type = VO;
		waiting_for_key = false;
        logger()->debug("readLine: found key: vo");
	}

    if (rx_ao_key.indexIn(line) >= 0) {
		reading_type = AO;
		waiting_for_key = false;
        logger()->debug("readLine: found key: ao");
	}

    if (rx_demuxer_key.indexIn(line) >= 0) {
		reading_type = DEMUXER;
		waiting_for_key = false;
        logger()->debug("readLine: found key: demuxer");
	}

    if (rx_ac_key.indexIn(line) >= 0) {
		reading_type = AC;
		waiting_for_key = false;
        logger()->debug("readLine: found key: ac");
	}

    if (rx_vc_key.indexIn(line) >= 0) {
		reading_type = VC;
		waiting_for_key = false;
        logger()->debug("readLines: found key: vc");
	}
}

bool TPlayerInfoMplayer::run(QString options) {
    logger()->debug("run: '" + options + "'");

	if (proc->state() == QProcess::Running) {
        logger()->warn("run: process already running");
		return false;
	}

	QStringList args = options.split(" ");

	proc->start(bin, args);
	if (!proc->waitForStarted()) {
        logger()->warn("run: process can't start!");
		return false;
	}

	//Wait until finish
	if (!proc->waitForFinished()) {
        logger()->warn("run: process didn't finish. Killing it...");
		proc->kill();
	}

    logger()->debug("run : terminating");

	QByteArray ba;
	while (proc->canReadLine()) {
		ba = proc->readLine();
		ba.replace("\n", "");
		ba.replace("\r", "");
		readLine(ba);
	}

	return true;
}

} // namespace Info
} // namespace Player

#include "moc_playerinfomplayer.cpp"
