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

#include "discname.h"
#include <QRegExp>
#include <QFileInfo>
#include <QApplication>


TDiscName::TDiscName() :
    title(0),
    valid(false) {
}

TDiscName::TDiscName(const TDiscName& disc) :
    title(disc.title),
    valid(disc.valid) {

    if (valid) {
        protocol = disc.protocol;
        device = disc.device;
    }
}

TDiscName::TDiscName(const QString& aprotocol,
                     int atitle,
                     const QString& adevice) :
    protocol(aprotocol),
    title(atitle),
    device(adevice),
    valid(true) {

    removeTrailingSlashFromDevice();
}

TDiscName::TDiscName(const QString& adevice, bool use_dvd_nav) :
    protocol(use_dvd_nav ? "dvdnav" : "dvd"),
    title(0),
    device(adevice),
    valid(true) {

    removeTrailingSlashFromDevice();
}

TDiscName::TDiscName(const QString& url) {

    // TODO: dvdread and title ranges dvd://1-99
    static QRegExp rx1("^(dvd|dvdnav|vcd|cdda|br)://(\\d+)/(.*)$", Qt::CaseInsensitive);
    static QRegExp rx2("^(dvd|dvdnav|vcd|cdda|br):///(.*)$", Qt::CaseInsensitive);
    static QRegExp rx3("^(dvd|dvdnav|vcd|cdda|br)://(\\d+)$", Qt::CaseInsensitive);
    static QRegExp rx4("^(dvd|dvdnav|vcd|cdda|br):(//)?$", Qt::CaseInsensitive);

    valid = false;

    if (rx1.indexIn(url) >= 0) {
        protocol = rx1.cap(1);
        title = rx1.cap(2).toInt();
        device = rx1.cap(3);
        valid = true;
    } else if (rx2.indexIn(url) >= 0) {
        protocol = rx2.cap(1);
        title = 0;
        device = rx2.cap(2);
        valid = true;
    } else if (rx3.indexIn(url) >= 0) {
        protocol = rx3.cap(1);
        title = rx3.cap(2).toInt();
        valid = true;
    } else if (rx4.indexIn(url) >= 0) {
        protocol = rx4.cap(1);
        title = 0;
        valid = true;
    }

    protocol = protocol.toLower();
    removeTrailingSlashFromDevice();
}

TDiscName::~TDiscName() {
}

QString TDiscName::displayName(bool addDevice) const {

    QString name;

    if (valid && !device.isEmpty()) {
        QFileInfo fi(device);
        QString deviceName = fi.fileName();
        // fileName() return empty if name ends in /
        if (deviceName.isEmpty()) {
            deviceName = device;
        }
        if (title > 0) {
            if (addDevice) {
                name = deviceName + " - ";
            }
            if (protocol == "cdda" || protocol == "vcd") {
                name += qApp->translate("TDiscName", "track %1").arg(title);
            } else {
                name += qApp->translate("TDiscName", "title %1").arg(title);
            }
        } else {
            name = deviceName;
        }
    }

    return name;
}

// This functions remove the trailing "/" from the device
// with one exception: from Windows drives letters (D:/ E:/...)
void TDiscName::removeTrailingSlashFromDevice() {

    if (device.endsWith("/")) {

#ifdef Q_OS_WIN
        static QRegExp r("^[A-Z]:/$");
        if (r.indexIn(device) < 0)
#endif
            device.chop(1);
    }
}

QString TDiscName::toString(bool add_zero_title) const {

    QString s;
    if (valid) {
        s = protocol + "://";
        if (title > 0 || (add_zero_title && title == 0)) {
            s += QString::number(title);
        }
        if (!device.isEmpty()) {
            s += "/" + device;
        }
    }
    return s;
}

TDiscName::TDisc TDiscName::protocolToTDisc(QString protocol) {

    protocol = protocol.toLower();
    if (protocol == "dvdnav")
        return DVDNAV;
    if (protocol == "vcd")
        return VCD;
    if (protocol == "cdda")
        return CDDA;
    if (protocol == "br")
        return BLURAY;

    return DVD;
}

TDiscName::TDisc TDiscName::disc() const {
    return protocolToTDisc(protocol);
}

