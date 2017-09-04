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

#include "helper.h"

#include <QFileInfo>
#include <QDir>
#include <QUrl>

#include "settings/preferences.h"
#include "wzdebug.h"


using namespace Settings;

LOG4QT_DECLARE_STATIC_LOGGER(logger, Helper)


int Helper::qtVersion() {

    QRegExp rx("(\\d+)\\.(\\d+)\\.(\\d+)");
    QString v(qVersion());

    int r = 0;
    if (rx.indexIn(v) >= 0) {
        int n1 = rx.cap(1).toInt();
        int n2 = rx.cap(2).toInt();
        int n3 = rx.cap(3).toInt();
        r = n1 * 1000 + n2 * 100 + n3;
    }

    return r;
}

// Return base name for url
QString Helper::nameForURL(QString url, bool extension) {

    if (url.isEmpty()) {
        return "";
    }

    QString name;
    QUrl qUrl(url);
    if (qUrl.scheme().toLower() == "file") {
        url = qUrl.toLocalFile();
    }

    QFileInfo fi(url);
    if (fi.exists()) {
        if (fi.isDir()) {
            name = fi.fileName(); // Returns "" when name ends with /
            if (name.isEmpty()) {
                name = fi.dir().dirName(); // Returns "" for root
                if (name.isEmpty()) {
                    name = QDir::separator();
                }
            }
        } else if (extension) {
            name = fi.fileName();
        } else {
            name = fi.completeBaseName();
        }
    } else {

#ifdef Q_OS_WIN
        // For non-existing local files on Windows qUrl will convert \ to %5C
        qUrl.setUrl(url.replace('\\', '/'));
#endif

        name = qUrl.toString(QUrl::RemoveScheme
                             | QUrl::RemoveAuthority
                             | QUrl::RemoveQuery
                             | QUrl::RemoveFragment
                             | QUrl::StripTrailingSlash);
        if (name.isEmpty()) {
            name = url;
        } else {
            fi.setFile(name);
            if (fi.isDir()) {
                name = fi.fileName();
                if (name.isEmpty()) {
                    name = QDir::separator();
                }
            } if (extension) {
                name = fi.fileName();
            } else {
                name = fi.completeBaseName();
            }
        }
    }

    WZTRACE("'" + url + "' to '" + name + "'");
    return name;
}

QString Helper::clean(const QString& name) {

    if (name.isEmpty()) {
        return "";
    }

    QString s = name;

    // \w Matches a word character (QChar::isLetterOrNumber(),
    //    QChar::isMark(), or '_')
    // \W Matches a non-word character

    s.replace("_", " ");

    // Replace dots not surrounded by whitespace with space
    static QRegExp rx1("([^\\s])\\.([^\\s])");
    s.replace(rx1, "\\1 \\2");

    // Surround hyphen with spaces
    static QRegExp rx2("([^\\s])-([^\\s])");
    s.replace(rx2, "\\1 - \\2");

    // Surround x with spaces
    static QRegExp rx3("(\\d)[xX](\\d)");
    s.replace(rx3, "\\1 x \\2");

    // Prefix digits not from know media extension with space
    static QRegExp rx4("([^\\d\\s\\WmMpPcCuU])(\\d+)");
    s.replace(rx4, "\\1 \\2");

    s = s.simplified();
    if (s.length() > 255) {
        s = s.left(252) + "...";
    }

    WZTRACE("'" + name + "' to '" + s + "'");
    return s;
}

QString Helper::cleanName(const QString& name) {
    return clean(name);
}

QString Helper::cleanTitle(const QString& title) {

    foreach(const QRegExp& rx, pref->rxTitleBlacklist) {
        if (rx.indexIn(title) >= 0) {
            WZINFO("'" + title + "' blacklisted on '" + rx.pattern() + "'");
            return "";
        }
    }

    return clean(title);
}
