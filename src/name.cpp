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

#include "name.h"

#include <QFileInfo>
#include <QDir>
#include <QUrl>

#include "settings/preferences.h"
#include "discname.h"
#include "wzdebug.h"
#include "config.h"


using namespace Settings;

LOG4QT_DECLARE_STATIC_LOGGER(logger, TName)


QString removeTrailingSlash(const QString& url) {

    QString s = url;
    do {
        QChar last = s.at(s.length() - 1);
        if (last == '/') {
            s.chop(1);
            if (!s.isEmpty()) {
                continue;
            }
        }

#ifdef Q_OS_WIN
        else if (last == '\\') {
            s.chop(1);
            if (!s.isEmpty()) {
                continue;
            }
        }
#endif

    } while (false);

    return s;
}

QString TName::nameForURL(QString url) {

    if (url.isEmpty()) {
        return url;
    }

    bool isWZPlaylist = false;
    QString name = url;
    if (name.endsWith(TConfig::WZPLAYLIST)) {
        name = name.left(name.length() - TConfig::WZPLAYLIST.length());
        if (name.isEmpty()) {
            // Current dir
            return ".";
        }
        isWZPlaylist = true;
        url = name;
    }

    name = removeTrailingSlash(name);
    if (name.isEmpty()) {
        // Root
        return url.right(1);
    }

    // Remove path
    int i = name.lastIndexOf('/');
    if (i < 0 && QDir::separator() != '/') {
        i = name.lastIndexOf(QDir::separator());
    }
    if (i >= 0) {
        name = name.mid(i + 1);
    } else {
        // No path, remove scheme
        QUrl u(url);
        QString s = u.scheme();
        if (!s.isEmpty()) {
            s = name.mid(s.length());
            // Remove : after scheme. / does not get here.
            if (s.startsWith(':')) {
                s = s.mid(1);
            }
            if (!s.isEmpty()) {
                name = s;
            }
        }
    }

    if (isWZPlaylist) {
        return name;
    }

    QUrl qUrl(url);
    if (!qUrl.scheme().isEmpty()) {
        if (qUrl.scheme() == "file") {
            url = qUrl.toLocalFile();
        } else {
            TDiscName disc(url);
            if (disc.valid) {
                return disc.displayName();
            }
        }
    }

    QFileInfo fi(url);
    if (fi.isDir()) {
        return name;
    }

    // Return completeBaseName
    fi.setFile(name);
    return fi.completeBaseName();
}

QString TName::clean(const QString& name) {

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

    // Surround hyphen with spaces, unless it might be a date: [^0-9]
    static QRegExp rx2("([^0-9][^\\s])-([^\\s])");
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

QString TName::cleanTitle(const QString& title) {

    for(int i = pref->rxTitleBlacklist.count() - 1; i >= 0; i--) {
        const QRegExp& rx = pref->rxTitleBlacklist.at(i);
        if (rx.indexIn(title) >= 0) {
            WZINFO("'" + title + "' blacklisted on '" + rx.pattern() + "'");
            return "";
        }
    }

    return clean(title);
}
