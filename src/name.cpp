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
#include "wzdebug.h"


using namespace Settings;

LOG4QT_DECLARE_STATIC_LOGGER(logger, TName)


QString removeTrailingSlash(const QString& url) {

    QString s = url;
    do {
        QChar last = s.at(s.length() - 1);
        if (last == '/') {
            s = s.left(s.length() - 1);
            if (!s.isEmpty()) {
                continue;
            }
        }

#ifdef Q_OS_WIN
        else if (last == '\\') {
            s = s.left(s.length() - 1);
            if (!s.isEmpty()) {
                continue;
            }
        }
#endif

    } while (false);

    return s;
}

QString TName::nameForURL(const QString& url) {

    if (url.isEmpty()) {
        return url;
    }

    QString name = removeTrailingSlash(url);
    if (name.isEmpty()) {
        return url.right(1);
    }

    int i = name.lastIndexOf('/');

#ifdef Q_OS_WIN
    if (i < 0) {
        i = name.lastIndexOf('\\');
    }
#endif

    if (i < 0) {
        // Remove scheme
        QUrl u(url);
        QString s = u.scheme();
        if (s.isEmpty()) {
            return name;
        }
        return name.mid(s.length() + 1);
    }

    // Remove path
    return name.mid(i + 1);
}

QString TName::baseNameForURL(const QString& url) {

    QString name = nameForURL(url);
    if (name.isEmpty()) {
        return name;
    }

    // Check url is dir.
    QChar last = url.at(url.length() - 1);
    if (last == '/') {
        return name;
    }

#ifdef Q_OS_WIN
    if (last == '\\') {
        return name;
    }
#endif

    QString dir = url;
    QUrl qUrl(dir);
    if (qUrl.scheme() == "file") {
        dir = qUrl.toLocalFile();
    }
    QFileInfo fi(dir);
    if (fi.isDir()) {
        return name;
    }

    // Assume file. Not necessarily true...
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
