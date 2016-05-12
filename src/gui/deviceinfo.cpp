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

#include "gui/deviceinfo.h"
#include <QProcess>
#include <QFile>
#include <QDebug>
#include "log4qt/logger.h"


namespace Gui {

#ifndef Q_OS_WIN

LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::TDeviceList)


TDeviceList TDeviceInfo::alsaDevices() {
    logger()->debug("alsaDevices");

	TDeviceList l;
	QRegExp rx_device("^card\\s([0-9]+).*\\[(.*)\\],\\sdevice\\s([0-9]+):");

	QProcess p;
	p.setProcessChannelMode(QProcess::MergedChannels);
	p.setEnvironment(QStringList() << "LC_ALL=C");
	p.start("aplay", QStringList() << "-l");

	if (p.waitForFinished()) {
		QByteArray line;
		while (p.canReadLine()) {
			line = p.readLine().trimmed();
            logger()->debug("alsaDevices: '" + QString(line) + "'");
			if (rx_device.indexIn(line) >= 0) {
				QString id = rx_device.cap(1);
				id.append(".");
				id.append(rx_device.cap(3));
				QString desc = rx_device.cap(2);
                logger()->debug("alsaDevices: found device: '" + id
                                + "' '" + desc + "'");
				l.append(TDeviceData(id, desc));
			}
		}
	} else {
        logger()->warn("alsaDevices: could not start aplay, error %1",
                       p.error());
	}

	return l;
}

TDeviceList TDeviceInfo::xvAdaptors() {
    logger()->debug("xvAdaptors");

	TDeviceList l;
	QRegExp rx_device("^Adaptor #([0-9]+): \"(.*)\"");

	QProcess p;
	p.setProcessChannelMode(QProcess::MergedChannels);
	p.setEnvironment(QProcess::systemEnvironment() << "LC_ALL=C");
	p.start("xvinfo");

	if (p.waitForFinished()) {
		while (p.canReadLine()) {
			QString s = QString::fromLocal8Bit(p.readLine()).trimmed();
            logger()->debug("xvAdaptors: '" + s + "'");
			if (rx_device.indexIn(s) >= 0) {
				QString id = rx_device.cap(1);
				QString desc = rx_device.cap(2);
                logger()->debug("xvAdaptors: found adaptor: '" + id
                                + " '" + desc + "'");
				l.append(TDeviceData(id, desc));
			}
		}
	} else {
        logger()->warn("xvAdaptors: could not start xvinfo, error %1",
                       p.error());
	}

	return l;
}

#endif

} // namespace Gui

