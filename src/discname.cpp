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

TDiscName::TDiscName(const QString& url) : valid(false) {

    // TODO: title ranges dvd://1-99
    static QRegExp rx("^(dvd|dvdnav|dvdread|vcd|cdda|br):(//(-?\\d*)(/(.*))?)?$",
                      Qt::CaseInsensitive);

    if (rx.indexIn(url) >= 0) {
        protocol = rx.cap(1).toLower();
        if (protocol == "dvdread") {
            protocol == "dvd";
        }
        title = rx.cap(3).toInt();
        device = rx.cap(5);
        valid = true;
    }

    removeTrailingSlashFromDevice();
}

QString TDiscName::tr(const char* s) {
    return qApp->translate("TDiscName", s);
}


QString TDiscName::displayName(bool addDevice /* = true */) const {

    QString name;

    if (valid) {
        QString deviceName;
        if (device.isEmpty()) {
            deviceName = protocol;
        } else {
            QFileInfo fi(device);
            // To be compatible with TName::nameForURL() and
            // TPlaylistItem::setFileInfo need to return completeBaseName() for
            // iso's here.
            if (fi.isFile()) {
                deviceName = fi.completeBaseName();
            } else {
                fi.fileName();
                // fileName() return empty if name ends in / like in / or c:/
                if (deviceName.isEmpty()) {
                    deviceName = device;
                }
            }
        }

        if (title < 0) {
            if (addDevice) {
                name += tr("Menu %1").arg(deviceName);
            } else {
                name += tr("Menu");
            }
        } else if (title == 0) {
            name = deviceName;
        } else {
            if (addDevice) {
                name = deviceName + " - ";
            }
            if (protocol == "cdda" || protocol == "vcd") {
                name += tr("Track %1").arg(title);
            } else {
                name += tr("Title %1").arg(title);
            }
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

QString TDiscName::toString(bool addMenuTitle, bool add_zero_title) const {

    QString s;
    if (valid) {
        s = protocol + "://";
        if (title > 0) {
            s += QString::number(title);
        } else if (title < 0 && addMenuTitle) {
            s += "-1";
        } else if (add_zero_title && title <= 0) {
            s += "0";
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

